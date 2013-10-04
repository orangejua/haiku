/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "MainWindow.h"

#include <map>

#include <Alert.h>
#include <Autolock.h>
#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <ScrollView.h>
#include <StringList.h>
#include <TabView.h>

#include <package/Context.h>
#include <package/manager/Exceptions.h>
#include <package/RefreshRepositoryRequest.h>
#include <package/PackageRoster.h>
#include "package/RepositoryCache.h"
#include <package/solver/SolverPackage.h>

#include "AutoDeleter.h"
#include "AutoLocker.h"
#include "DecisionProvider.h"
#include "FilterView.h"
#include "JobStateListener.h"
#include "PackageInfoView.h"
#include "PackageListView.h"
#include "PackageManager.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWindow"


enum {
	MSG_MODEL_WORKER_DONE = 'mmwd',
	MSG_REFRESH_DEPOTS = 'mrdp',
	MSG_PACKAGE_STATE_CHANGED = 'mpsc'
};


using namespace BPackageKit;
using BManager::BPrivate::BException;
using BManager::BPrivate::BFatalErrorException;


typedef std::map<BString, PackageInfoRef> PackageInfoMap;
typedef std::map<BString, BObjectList<PackageInfo> > PackageLocationMap;
typedef std::map<BString, DepotInfo> DepotInfoMap;


struct RefreshWorkerParameters {
	MainWindow* window;
	bool forceRefresh;

	RefreshWorkerParameters(MainWindow* window, bool forceRefresh)
		:
		window(window),
		forceRefresh(forceRefresh)
		{
		}
};


MainWindow::MainWindow(BRect frame, const BMessage& settings)
	:
	BWindow(frame, B_TRANSLATE_SYSTEM_NAME("HaikuDepot"),
		B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fTerminating(false),
	fModelWorker(B_BAD_THREAD_ID)
{
	BMenuBar* menuBar = new BMenuBar(B_TRANSLATE("Main Menu"));
	_BuildMenu(menuBar);

	fFilterView = new FilterView(fModel);
	fPackageListView = new PackageListView(fModel.Lock());
	fPackageInfoView = new PackageInfoView(fModel.Lock(), this);

	fSplitView = new BSplitView(B_VERTICAL, 5.0f);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		.Add(menuBar)
		.Add(fFilterView)
		.AddSplit(fSplitView)
			.AddGroup(B_VERTICAL)
				.Add(fPackageListView)
				.SetInsets(
					B_USE_DEFAULT_SPACING, 0.0f,
					B_USE_DEFAULT_SPACING, 0.0f)
			.End()
			.Add(fPackageInfoView)
		.End()
	;

	fSplitView->SetCollapsible(0, false);
	fSplitView->SetCollapsible(1, false);

	// Restore settings
	BMessage columnSettings;
	if (settings.FindMessage("column settings", &columnSettings) == B_OK)
		fPackageListView->LoadState(&columnSettings);

	_StartRefreshWorker();

	fPendingActionsSem = create_sem(0, "PendingPackageActions");
	if (fPendingActionsSem >= 0) {
		fPendingActionsWorker = spawn_thread(&_PackageActionWorker,
			"Planet Express", B_NORMAL_PRIORITY, this);
		if (fPendingActionsWorker >= 0)
			resume_thread(fPendingActionsWorker);
	}
}


MainWindow::~MainWindow()
{
	fTerminating = true;
	if (fModelWorker > 0) {
		status_t result;
		wait_for_thread(fModelWorker, &result);
	}

	delete_sem(fPendingActionsSem);
	status_t result;
	wait_for_thread(fPendingActionsWorker, &result);
}


bool
MainWindow::QuitRequested()
{
	BMessage settings;
	StoreSettings(settings);

	BMessage message(MSG_MAIN_WINDOW_CLOSED);
	message.AddMessage("window settings", &settings);

	be_app->PostMessage(&message);

	return true;
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_MODEL_WORKER_DONE:
		{
			fModelWorker = B_BAD_THREAD_ID;
			_AdoptModel();
			break;
		}
		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
			// TODO: ?
			break;

		case MSG_REFRESH_DEPOTS:
		{
			_StartRefreshWorker(true);
			break;
		}
		case MSG_PACKAGE_SELECTED:
		{
			BString title;
			if (message->FindString("title", &title) == B_OK) {
				int count = fVisiblePackages.CountItems();
				for (int i = 0; i < count; i++) {
					const PackageInfoRef& package
						= fVisiblePackages.ItemAtFast(i);
					if (package.Get() != NULL && package->Title() == title) {
						_AdoptPackage(package);
						break;
					}
				}
			} else {
				_ClearPackage();
			}
			break;
		}

		case MSG_CATEGORY_SELECTED:
		{
			BString name;
			if (message->FindString("name", &name) != B_OK)
				name = "";
			fModel.SetCategory(name);
			_AdoptModel();
			break;
		}

		case MSG_DEPOT_SELECTED:
		{
			BString name;
			if (message->FindString("name", &name) != B_OK)
				name = "";
			fModel.SetDepot(name);
			_AdoptModel();
			break;
		}

		case MSG_SEARCH_TERMS_MODIFIED:
		{
			// TODO: Do this with a delay!
			BString searchTerms;
			if (message->FindString("search terms", &searchTerms) != B_OK)
				searchTerms = "";
			fModel.SetSearchTerms(searchTerms);
			_AdoptModel();
			break;
		}

		case MSG_PACKAGE_STATE_CHANGED:
		{
			PackageInfo* info;
			if (message->FindPointer("package", (void **)&info) == B_OK) {
				PackageInfoRef ref(info, true);
				fModel.SetPackageState(ref, ref->State());
			}
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
MainWindow::StoreSettings(BMessage& settings) const
{
	settings.AddRect("window frame", Frame());

	BMessage columnSettings;
	fPackageListView->SaveState(&columnSettings);

	settings.AddMessage("column settings", &columnSettings);
}


void
MainWindow::PackageChanged(const PackageInfoEvent& event)
{
	if ((event.Changes() & PKG_CHANGED_STATE) != 0) {
		PackageInfoRef ref(event.Package());
		BMessage message(MSG_PACKAGE_STATE_CHANGED);
		message.AddPointer("package", ref.Get());
		ref.Detach();
			// reference needs to be released by MessageReceived();
		PostMessage(&message);
	}
}


status_t
MainWindow::SchedulePackageActions(PackageActionList& list)
{
	AutoLocker<BLocker> lock(&fPendingActionsLock);
	for (int32 i = 0; i < list.CountItems(); i++) {
		if (!fPendingActions.Add(list.ItemAtFast(i))) {
			return B_NO_MEMORY;
		}
	}

	return release_sem_etc(fPendingActionsSem, list.CountItems(), 0);
}


Model*
MainWindow::GetModel()
{
	return &fModel;
}


void
MainWindow::_BuildMenu(BMenuBar* menuBar)
{
	BMenu* menu = new BMenu(B_TRANSLATE("Package"));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Refresh depots"),
			new BMessage(MSG_REFRESH_DEPOTS)));
	menuBar->AddItem(menu);

}


void
MainWindow::_AdoptModel()
{
	fVisiblePackages = fModel.CreatePackageList();

	fPackageListView->Clear();
	for (int32 i = 0; i < fVisiblePackages.CountItems(); i++) {
		BAutolock _(fModel.Lock());
		fPackageListView->AddPackage(fVisiblePackages.ItemAtFast(i));
	}
}


void
MainWindow::_AdoptPackage(const PackageInfoRef& package)
{
	fPackageInfoView->SetPackage(package);
	fModel.PopulatePackage(package);
}


void
MainWindow::_ClearPackage()
{
	fPackageInfoView->Clear();
}


void
MainWindow::_RefreshRepositories(bool force)
{
	BPackageRoster roster;
	BStringList repositoryNames;

	status_t result = roster.GetRepositoryNames(repositoryNames);
	if (result != B_OK)
		return;

	DecisionProvider decisionProvider;
	JobStateListener listener;
	BContext context(decisionProvider, listener);

	BRepositoryCache cache;
	for (int32 i = 0; i < repositoryNames.CountStrings(); ++i) {
		const BString& repoName = repositoryNames.StringAt(i);
		BRepositoryConfig repoConfig;
		result = roster.GetRepositoryConfig(repoName, &repoConfig);
		if (result != B_OK) {
			// TODO: notify user
			continue;
		}

		if (roster.GetRepositoryCache(repoName, &cache) != B_OK
			|| force) {
			try {
				BRefreshRepositoryRequest refreshRequest(context, repoConfig);

				result = refreshRequest.Process();
			} catch (BFatalErrorException ex) {
				BString message(B_TRANSLATE("An error occurred while "
					"refreshing the repository: %error% (%details%)"));
 				message.ReplaceFirst("%error%", ex.Message());
				message.ReplaceFirst("%details%", ex.Details());
				_NotifyUser("Error", message.String());
			} catch (BException ex) {
				BString message(B_TRANSLATE("An error occurred while "
					"refreshing the repository: %error%"));
				message.ReplaceFirst("%error%", ex.Message());
				_NotifyUser("Error", message.String());
			}
		}
	}
}


void
MainWindow::_RefreshPackageList()
{
	BPackageRoster roster;
	BStringList repositoryNames;

	status_t result = roster.GetRepositoryNames(repositoryNames);
	if (result != B_OK)
		return;

	DepotInfoMap depots;
	for (int32 i = 0; i < repositoryNames.CountStrings(); i++) {
		const BString& repoName = repositoryNames.StringAt(i);
		depots[repoName] = DepotInfo(repoName);
	}

	PackageManager manager(B_PACKAGE_INSTALLATION_LOCATION_HOME);
	try {
		manager.Init(PackageManager::B_ADD_INSTALLED_REPOSITORIES
			| PackageManager::B_ADD_REMOTE_REPOSITORIES);
	} catch (BException ex) {
		BString message(B_TRANSLATE("An error occurred while "
			"initializing the package manager: %message%"));
		message.ReplaceFirst("%message%", ex.Message());
		_NotifyUser("Error", message.String());
		return;
	}

	BObjectList<BSolverPackage> packages;
	result = manager.Solver()->FindPackages("",
		BSolver::B_FIND_CASE_INSENSITIVE | BSolver::B_FIND_IN_NAME
			| BSolver::B_FIND_IN_SUMMARY | BSolver::B_FIND_IN_DESCRIPTION
			| BSolver::B_FIND_IN_PROVIDES,
		packages);
	if (result != B_OK) {
		// TODO: notify user
		return;
	}

	if (packages.IsEmpty())
		return;

	PackageLocationMap packageLocations;
	PackageInfoMap foundPackages;
		// if a given package is installed locally, we will potentially
		// get back multiple entries, one for each local installation
		// location, and one for each remote repository the package
		// is available in. The above map is used to ensure that in such
		// cases we consolidate the information, rather than displaying
		// duplicates
	for (int32 i = 0; i < packages.CountItems(); i++) {
		BSolverPackage* package = packages.ItemAt(i);
		const BPackageInfo& repoPackageInfo = package->Info();
		PackageInfoRef modelInfo;
		PackageInfoMap::iterator it = foundPackages.find(
			repoPackageInfo.Name());
		if (it != foundPackages.end())
			modelInfo.SetTo(it->second);
		else {
			BString publisherURL;
			if (repoPackageInfo.URLList().CountStrings() > 0)
				publisherURL = repoPackageInfo.URLList().StringAt(0);
			const BStringList& rightsList = repoPackageInfo.CopyrightList();
			BString publisherName = repoPackageInfo.Vendor();
			if (rightsList.CountStrings() > 0)
				publisherName = rightsList.StringAt(0);
			modelInfo.SetTo(new(std::nothrow) PackageInfo(NULL,
					repoPackageInfo.Name(),
					repoPackageInfo.Version().ToString(),
					PublisherInfo(BitmapRef(), publisherName,
					"", publisherURL), repoPackageInfo.Summary(),
					repoPackageInfo.Description(), ""),
				true);

			if (modelInfo.Get() == NULL)
				return;

			foundPackages[repoPackageInfo.Name()] = modelInfo;
		}

		modelInfo->AddListener(this);

		BSolverRepository* repository = package->Repository();
		if (dynamic_cast<BPackageManager::RemoteRepository*>(repository)
				!= NULL) {
			depots[repository->Name()].AddPackage(modelInfo);
		} else {
			const char* installationLocation = NULL;
			if (repository == static_cast<const BSolverRepository*>(
					manager.SystemRepository())) {
				installationLocation = "system";
			} else if (repository == static_cast<const BSolverRepository*>(
					manager.HomeRepository())) {
				installationLocation = "home";
			}

			if (installationLocation != NULL) {
				packageLocations[installationLocation].AddItem(
					modelInfo.Get());
			}
		}
	}

	BAutolock lock(fModel.Lock());

	fModel.Clear();

	for (DepotInfoMap::iterator it = depots.begin(); it != depots.end();
		++it) {
		fModel.AddDepot(it->second);
	}

	for (PackageLocationMap::iterator it = packageLocations.begin();
		it != packageLocations.end(); ++it) {
		for (int32 i = 0; i < it->second.CountItems(); i++) {
			fModel.SetPackageState(it->second.ItemAt(i), ACTIVATED);
				// TODO: indicate the specific installation location
				// and verify that the package is in fact activated
				// by querying the package roster
		}
	}
}


void
MainWindow::_StartRefreshWorker(bool force)
{
	if (fModelWorker != B_BAD_THREAD_ID)
		return;

	RefreshWorkerParameters* parameters = new(std::nothrow)
		RefreshWorkerParameters(this, force);
	if (parameters == NULL)
		return;

	ObjectDeleter<RefreshWorkerParameters> deleter(parameters);
	fModelWorker = spawn_thread(&_RefreshModelThreadWorker, "model loader",
		B_LOW_PRIORITY, parameters);

	if (fModelWorker > 0) {
		deleter.Detach();
		resume_thread(fModelWorker);
	}
}


status_t
MainWindow::_RefreshModelThreadWorker(void* arg)
{
	RefreshWorkerParameters* parameters
		= reinterpret_cast<RefreshWorkerParameters*>(arg);
	MainWindow* mainWindow = parameters->window;
	ObjectDeleter<RefreshWorkerParameters> deleter(parameters);

	BMessenger messenger(mainWindow);

	mainWindow->_RefreshRepositories(parameters->forceRefresh);

	if (mainWindow->fTerminating)
		return B_OK;

	mainWindow->_RefreshPackageList();

	messenger.SendMessage(MSG_MODEL_WORKER_DONE);

	return B_OK;
}


status_t
MainWindow::_PackageActionWorker(void* arg)
{
	MainWindow* window = reinterpret_cast<MainWindow*>(arg);

	while (acquire_sem(window->fPendingActionsSem) == B_OK) {
		PackageActionRef ref;
		{
			AutoLocker<BLocker> lock(&window->fPendingActionsLock);
			ref = window->fPendingActions.ItemAt(0);
			if (ref.Get() == NULL)
				break;
			window->fPendingActions.Remove(0);
		}

		ref->Perform();
	}

	return 0;
}


void
MainWindow::_NotifyUser(const char* title, const char* message)
{
	BAlert *alert = new(std::nothrow) BAlert(title, message,
		B_TRANSLATE("Close"));

	if (alert != NULL)
		alert->Go();
}

