/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__PACKAGE_FILE_HEAP_WRITER_H_
#define _PACKAGE__HPKG__PRIVATE__PACKAGE_FILE_HEAP_WRITER_H_


#include <Array.h>
#include <package/hpkg/DataWriters.h>
#include <package/hpkg/PackageFileHeapAccessorBase.h>


namespace BPrivate {
	template<typename Value> class RangeArray;
}


namespace BPackageKit {

namespace BHPKG {


class BDataReader;
class BErrorOutput;


namespace BPrivate {


class PackageFileHeapReader;


class PackageFileHeapWriter : public PackageFileHeapAccessorBase,
	private AbstractDataWriter {
public:
								PackageFileHeapWriter(BErrorOutput* errorOutput,
									int fd, off_t heapOffset,
									int32 compressionLevel);
								~PackageFileHeapWriter();

			void				Init();
			void				Reinit(PackageFileHeapReader* heapReader);

			AbstractDataWriter* DataWriter()
									{ return this; }

			status_t			AddData(BDataReader& dataReader, off_t size,
									uint64& _offset);
			void				RemoveDataRanges(
									const ::BPrivate::RangeArray<uint64>&
										ranges);
									// doesn't truncate the file
			status_t			Finish();

protected:
	virtual	status_t			ReadAndDecompressChunk(size_t chunkIndex,
									void* compressedDataBuffer,
									void* uncompressedDataBuffer);

private:
	// AbstractDataWriter
	virtual	status_t			WriteDataNoThrow(const void* buffer,
									size_t size);

private:
			struct Chunk;
			struct ChunkSegment;
			struct ChunkBuffer;

			friend struct ChunkBuffer;

private:
			void				_Uninit();

			status_t			_FlushPendingData();
			status_t			_WriteChunk(const void* data, size_t size,
									bool mayCompress);
			status_t			_WriteDataCompressed(const void* data,
									size_t size);
			status_t			_WriteDataUncompressed(const void* data,
									size_t size);

			void				_PushChunks(ChunkBuffer& chunkBuffer,
									uint64 startOffset, uint64 endOffset);
			void				_UnwriteLastPartialChunk();

private:
			void*				fPendingDataBuffer;
			void*				fCompressedDataBuffer;
			size_t				fPendingDataSize;
			Array<uint64>		fOffsets;
			int32				fCompressionLevel;
};


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__PACKAGE_FILE_HEAP_WRITER_H_
