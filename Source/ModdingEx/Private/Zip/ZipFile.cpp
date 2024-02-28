#include "Zip/ZipFile.h"

#include "zip.h"

FZipError CreateError(const zip_error_t Error)
{
	FZipError ZipError{};
	ZipError.ErrorCode = Error.zip_err;
	if (Error.str)
	{
		ZipError.Description = FString(Error.str);
	}

	return ZipError;
}


bool FZipBuffer::TryCreateZipBuffer(const TArray<uint8>& Data, FZipBuffer& ZipBuffer, FZipError& Error)
{
	zip_error_t ZipError{};
	zip_source_t* Source = zip_source_buffer_create(Data.GetData(), Data.Num(), 0, &ZipError);

	if (!Source)
	{
		Error = CreateError(ZipError);
		return false;
	}

	ZipBuffer = FZipBuffer(Source);
	return true;
}

FZipBuffer::~FZipBuffer()
{
	if (Source)
	{
		zip_source_close(Source);
	}
}

bool FZipFile::TryCreateZipFile(const TArray<uint8>& Data, FZipFile& ZipFile, FZipError& Error)
{
	FZipBuffer Buffer{};
	if (!FZipBuffer::TryCreateZipBuffer(Data, Buffer, Error))
	{
		return false;
	}

	zip_error_t ZipError{};

	zip_t* Zip = zip_open_from_source(Buffer.Source, ZIP_RDONLY, &ZipError);
	if (!Zip)
	{
		Error = CreateError(ZipError);
		return false;
	}

	ZipFile = FZipFile(MoveTemp(Buffer), Zip);
	return true;
}

TOptional<FZipEntry> FZipFile::GetEntry(const FString& Name, FZipError& Error) const
{
	if (!Zip) return NullOpt;

	const int64 EntryIndex = zip_name_locate(Zip, TCHAR_TO_ANSI(*Name), 0);
	if (EntryIndex == -1)
	{
		return NullOpt;
	}

	return GetEntryIndex(EntryIndex, Error);
}

TOptional<FZipEntry> FZipFile::GetEntryIndex(uint64 Index, FZipError& Error) const
{
	zip_stat_t EntryStat{};
	if (zip_stat_index(Zip, Index, 0, &EntryStat) != 0)
	{
		return NullOpt;
	}

	zip_file* File = zip_fopen_index(Zip, Index, 0);
	if (!File)
	{
		const zip_error_t* ZipError = zip_get_error(Zip);
		Error = CreateError(*ZipError);
		return NullOpt;
	}

	FZipEntry Entry{};
	if (EntryStat.name)
	{
		Entry.Name = FString(EntryStat.name);
	}
	Entry.Index = Index;
	Entry.DecompressedSize = EntryStat.size;
	Entry.CompressedSize = EntryStat.comp_size;
	Entry.File = File;

	return Entry;
}

TArray<FZipEntry> FZipFile::GetEntries(FZipError& Error) const
{
	const int64 NumEntries = zip_get_num_entries(Zip, 0);

	TArray<FZipEntry> Entries{};
	FZipError ZipError{};

	for (int64 EntryIndex = 0; EntryIndex < NumEntries; EntryIndex++)
	{
		TOptional<FZipEntry> Entry = GetEntryIndex(EntryIndex, ZipError);

		if (!Entry)
		{
			if (ZipError.ErrorCode == ZIP_ER_DELETED)
			{
				continue;
			}
			Error = ZipError;
			return {};
		}

		Entries.Add(MoveTemp(*Entry));
	}

	return Entries;
}


TArray<uint8> FZipFile::ReadEntry(const FZipEntry& Entry) const
{
	if (!Zip) return {};

	TArray<uint8> Array{};
	Array.Init(0, Entry.DecompressedSize);

	const int64 ReadBytes = zip_fread(Entry.File, Array.GetData(), Entry.DecompressedSize);
	if (ReadBytes == -1)
	{
		return {};
	}

	return Array;
}


FZipFile::~FZipFile()
{
	if (Zip)
	{
		zip_close(Zip);
	}
}
