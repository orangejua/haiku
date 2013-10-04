/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 *		Rene Gollent <rene@gollent.com>
 */
#ifndef _PACKAGE__MANAGER__PRIVATE__PACKAGE_MANAGER_H_
#define _PACKAGE__MANAGER__PRIVATE__PACKAGE_MANAGER_H_


#include <Directory.h>
#include <ObjectList.h>
#include <package/Context.h>
#include <package/PackageDefs.h>
#include <package/PackageRoster.h>
#include <package/RepositoryConfig.h>
#include <package/solver/Solver.h>
#include <package/solver/SolverRepository.h>

#include <package/ActivationTransaction.h>
#include <package/DaemonClient.h>
#include <package/Job.h>


namespace BPackageKit {

namespace BManager {

namespace BPrivate {


using BPackageKit::BPrivate::BActivationTransaction;
using BPackageKit::BPrivate::BDaemonClient;


class BPackageManager : protected BJobStateListener {
public:
			class RemoteRepository;
			class InstalledRepository;
			class Transaction;
			class InstallationInterface;
			class ClientInstallationInterface;
			class UserInteractionHandler;

			typedef BObjectList<RemoteRepository> RemoteRepositoryList;
			typedef BObjectList<InstalledRepository> InstalledRepositoryList;
			typedef BObjectList<BSolverPackage> PackageList;
			typedef BObjectList<Transaction> TransactionList;

			enum {
				B_ADD_INSTALLED_REPOSITORIES	= 0x01,
				B_ADD_REMOTE_REPOSITORIES		= 0x02,
				B_REFRESH_REPOSITORIES			= 0x04,
			};

public:
								BPackageManager(
									BPackageInstallationLocation location);
	virtual						~BPackageManager();

			void				Init(uint32 flags);

			BSolver*			Solver() const
									{ return fSolver; }

			const InstalledRepository* SystemRepository() const
									{ return fSystemRepository; }
			const InstalledRepository* HomeRepository() const
									{ return fHomeRepository; }
			const InstalledRepositoryList& InstalledRepositories() const
									{ return fInstalledRepositories; }
			const RemoteRepositoryList& OtherRepositories() const
									{ return fOtherRepositories; }

			void				Install(const char* const* packages,
									int packageCount);
			void				Install(const BSolverPackageSpecifierList&
									packages);
			void				Uninstall(const char* const* packages,
									int packageCount);
			void				Uninstall(const BSolverPackageSpecifierList&
									packages);
			void				Update(const char* const* packages,
									int packageCount);
			void				Update(const BSolverPackageSpecifierList&
									packages);

			void				VerifyInstallation();


	virtual	status_t			DownloadPackage(const BString& fileURL,
									const BEntry& targetEntry,
									const BString& checksum);
	virtual	status_t			RefreshRepository(
									const BRepositoryConfig& repoConfig);

protected:
			InstalledRepository& InstallationRepository();

protected:
			// BJobStateListener
	virtual	void				JobStarted(BJob* job);
	virtual	void				JobProgress(BJob* job);
	virtual	void				JobSucceeded(BJob* job);

private:
			void				_HandleProblems();
			void				_AnalyzeResult();
			void				_ConfirmChanges(bool fromMostSpecific = false);
			void				_ApplyPackageChanges(
									bool fromMostSpecific = false);
			void				_PreparePackageChanges(
									InstalledRepository&
										installationRepository);
			void				_CommitPackageChanges(Transaction& transaction);

			void				_ClonePackageFile(
									InstalledRepository* repository,
									const BString& fileName,
							 		const BEntry& entry);
			int32				_FindBasePackage(const PackageList& packages,
									const BPackageInfo& info);

			void				_AddInstalledRepository(
									InstalledRepository* repository);
			void				_AddRemoteRepository(BPackageRoster& roster,
									const char* name, bool refresh);
			status_t			_GetRepositoryCache(BPackageRoster& roster,
									const BRepositoryConfig& config,
									bool refresh, BRepositoryCache& _cache);

			bool				_NextSpecificInstallationLocation();

protected:
			BPackageInstallationLocation fLocation;
			BSolver*			fSolver;
			InstalledRepository* fSystemRepository;
			InstalledRepository* fHomeRepository;
			InstalledRepositoryList fInstalledRepositories;
			RemoteRepositoryList fOtherRepositories;
			TransactionList		fTransactions;

			// must be set by the derived class
			InstallationInterface* fInstallationInterface;
			UserInteractionHandler* fUserInteractionHandler;
};


class BPackageManager::RemoteRepository : public BSolverRepository {
public:
								RemoteRepository(
									const BRepositoryConfig& config);

			const BRepositoryConfig& Config() const;

private:
			BRepositoryConfig	fConfig;
};


class BPackageManager::InstalledRepository : public BSolverRepository {
public:
			typedef BObjectList<BSolverPackage> PackageList;

public:
								InstalledRepository(const char* name,
									BPackageInstallationLocation location,
									int32 priority);

			BPackageInstallationLocation Location() const
									{ return fLocation; }
			const char*			InitialName() const
									{ return fInitialName; }
			int32				InitialPriority() const
									{ return fInitialPriority; }

			void				DisablePackage(BSolverPackage* package);
									// throws, if already disabled
			bool				EnablePackage(BSolverPackage* package);
									// returns whether it was disabled

			PackageList&		PackagesToActivate()
									{ return fPackagesToActivate; }
			PackageList&		PackagesToDeactivate()
									{ return fPackagesToDeactivate; }

			bool				HasChanges() const;
			void				ApplyChanges();

private:
			PackageList			fDisabledPackages;
			PackageList			fPackagesToActivate;
			PackageList			fPackagesToDeactivate;
			const char*			fInitialName;
			BPackageInstallationLocation fLocation;
			int32				fInitialPriority;
};


class BPackageManager::Transaction {
public:
								Transaction(InstalledRepository& repository);
								~Transaction();

			InstalledRepository& Repository()
									{ return fRepository; }
			BActivationTransaction& ActivationTransaction()
									{ return fTransaction; }
			BDirectory&			TransactionDirectory()
									{ return fTransactionDirectory; }

private:
			InstalledRepository& fRepository;
			BActivationTransaction fTransaction;
			BDirectory			fTransactionDirectory;
};


class BPackageManager::InstallationInterface {
public:
	virtual						~InstallationInterface();

	virtual	void				InitInstalledRepository(
									InstalledRepository& repository) = 0;
	virtual	void				ResultComputed(InstalledRepository& repository);

	virtual	status_t			PrepareTransaction(Transaction& transaction)
									= 0;
	virtual	status_t			CommitTransaction(Transaction& transaction,
									BDaemonClient::BCommitTransactionResult&
										_result) = 0;
};


class BPackageManager::ClientInstallationInterface
	: public InstallationInterface {
public:
								ClientInstallationInterface();
	virtual						~ClientInstallationInterface();

	virtual	void				InitInstalledRepository(
									InstalledRepository& repository);

	virtual	status_t			PrepareTransaction(Transaction& transaction);
	virtual	status_t			CommitTransaction(Transaction& transaction,
									BDaemonClient::BCommitTransactionResult&
										_result);

private:
			BDaemonClient		fDaemonClient;
};


class BPackageManager::UserInteractionHandler {
public:
	virtual						~UserInteractionHandler();

	virtual	void				HandleProblems() = 0;
	virtual	void				ConfirmChanges(bool fromMostSpecific) = 0;

	virtual	void				Warn(status_t error, const char* format, ...)
									= 0;

	virtual	void				ProgressPackageDownloadStarted(
									const char* packageName) = 0;
	virtual	void				ProgressPackageDownloadActive(
									const char* packageName,
									float completionPercentage) = 0;
	virtual	void				ProgressPackageDownloadComplete(
									const char* packageName) = 0;
	virtual	void				ProgressPackageChecksumStarted(
									const char* title) = 0;
	virtual	void				ProgressPackageChecksumComplete(
									const char* title) = 0;

	virtual	void				ProgressStartApplyingChanges(
									InstalledRepository& repository) = 0;
	virtual	void				ProgressTransactionCommitted(
									InstalledRepository& repository,
									const char* transactionDirectoryName) = 0;
	virtual	void				ProgressApplyingChangesDone(
									InstalledRepository& repository) = 0;
};


}	// namespace BPrivate

}	// namespace BManager

}	// namespace BPackageKit


#endif	// _PACKAGE__MANAGER__PRIVATE__PACKAGE_MANAGER_H_
