#pragma once
#include <utility>

#include "zip.h"


struct FZipError
{
	int32 ErrorCode;
	TOptional<FString> Description;
};

class FZipBuffer
{
public:
	/**
	 * Try to create a zip buffer
	 *
	 * @param Data Buffer to use, must be valid for the full lifetime of FZipBuffer 
	 * @param ZipBuffer Result buffer, gets set if creation succeeded
	 * @param Error Error information, gets set if creation failed
	 * @return Returns if the creation was successful
	 */
	static bool TryCreateZipBuffer(const TArray<uint8>& Data, FZipBuffer& ZipBuffer, FZipError& Error);

public:
	FZipBuffer() = default;

	FZipBuffer(const FZipBuffer&) = delete;
	FZipBuffer& operator=(const FZipBuffer&) = delete;

	FZipBuffer(FZipBuffer&& Other) noexcept : Source(std::exchange(Other.Source, nullptr))
	{
	}

	FZipBuffer& operator=(FZipBuffer&& Other) noexcept
	{
		Swap(Source, Other.Source);
		return *this;
	}

	~FZipBuffer();

private:
	FZipBuffer(zip_source_t* Source) : Source(Source)
	{
	}

private:
	zip_source_t* Source;

private:
	friend class FZipFile;
};

struct FZipEntry
{
	TOptional<FString> Name;
	uint64 Index;
	uint64 DecompressedSize;
	uint64 CompressedSize;

public:
	FZipEntry() = default;

	FZipEntry(const FZipEntry&) = delete;
	FZipEntry& operator=(const FZipEntry&) = delete;

	FZipEntry(FZipEntry&& Other) noexcept : Name(std::exchange(Other.Name, {})), Index(std::exchange(Other.Index, {})),
	                                        DecompressedSize(std::exchange(Other.DecompressedSize, {})),
	                                        CompressedSize(std::exchange(Other.CompressedSize, {})),
	                                        File(std::exchange(Other.File, nullptr))
	{
	}

	FZipEntry& operator=(FZipEntry&& Other) noexcept
	{
		Swap(Name, Other.Name);
		Swap(Index, Other.Index);
		Swap(DecompressedSize, Other.DecompressedSize);
		Swap(CompressedSize, Other.CompressedSize);
		Swap(File, Other.File);
		return *this;
	}

	~FZipEntry()
	{
		if (File)
		{
			zip_fclose(File);
		}
	}

private:
	zip_file_t* File;

private:
	friend class FZipFile;
};

class FZipFile
{
public:
	/**
	 * Try to create a zip file
	 * 
	 * @param Data Buffer to use, must be valid for the full lifetime of FZipFile
	 * @param ZipFile Result file, gets set if creation succeeded
	 * @param Error Error, gets set if creation failed
	 * @return Returns if the creation was successful
	 */
	static bool TryCreateZipFile(const TArray<uint8>& Data, FZipFile& ZipFile, FZipError& Error);

public:
	/**
	 * 
	 * @param Name Entry name
	 * @param Error Error, gets set if getting the entry failed
	 * @return Entry, if one exists
	 */
	 TOptional<FZipEntry> GetEntry(const FString& Name, FZipError& Error) const;

	/**
	 * 
	 * @param Index Entry index
	 * @param Error Error, gets set if getting the entry failed
	 * @return Entry, if one exists
	 */
	TOptional<FZipEntry> GetEntryIndex(uint64 Index, FZipError& Error) const;
	
	/**
	 * Get all entries from this zip 
	 * 
	 * @param Error Error, gets set if getting entries
	 * @return All zip entries
	 */
	TArray<FZipEntry> GetEntries(FZipError& Error) const;
	
	TArray<uint8> ReadEntry(const FZipEntry& Entry) const;

public:
	FZipFile() = default;

	FZipFile(const FZipFile&) = delete;
	FZipFile& operator=(const FZipFile&) = delete;

	FZipFile(FZipFile&& Other) noexcept : Buffer(std::exchange(Other.Buffer, {})),
	                                      Zip(std::exchange(Other.Zip, nullptr))
	{
	}

	FZipFile& operator=(FZipFile&& Other) noexcept
	{
		Swap(Buffer, Other.Buffer);
		Swap(Zip, Other.Zip);
		return *this;
	}

	~FZipFile();

private:
	FZipFile(FZipBuffer Buffer, zip_t* Zip) : Buffer(MoveTemp(Buffer)), Zip(Zip)
	{
	}

private:
	FZipBuffer Buffer;
	zip_t* Zip;
};
