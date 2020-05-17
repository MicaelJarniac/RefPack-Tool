/* $Id$ */

// compress.cpp : Defines the entry point for the console application.
//

#include <conio.h>
#include "../include/misc.h"
#include "../include/files.h"
#include "../include/refpack.h"

void print_header();
void print_help();

int main(int argc, char* argv[])
{
	ErrInfo ei;

	print_header();

	if (argc < 2) {
		print_help();
		return -1;
	}

	BinaryFile src;
	src.Open(ei, argv[1], BinaryFile::omRead);

	//	uint32 num_bytes = f.Length(ei);
	uint64 num_bytes = src.m_filesize;


	uint64 out_length = 0;

	if (ei.Succeeded()) {
		if (num_bytes < 10) {
			printf("Makes no sense to compress such a small file.\n");
		} else {
			uint32 num_bytes_10 = (uint32)MinT(num_bytes, 10UI64);
			CBlobT<byte> src_blob_10;
			src.Read(ei, src_blob_10.GrowRawSize(num_bytes_10), num_bytes_10);
			src.Seek(ei, 0);
			if (RefPack::IsCompressed(src_blob_10.Data())) {
				printf("Already compressed.\n");
			} else {
				uint32 t = GetTickCount();

				if (argc > 2) {
					printf("Compressing into file ");
					BinaryFile dst;
					dst.Open(ei, argv[2], BinaryFile::omWrite);
					RefPackCompress<BinaryFile, BinaryFile> *c = new RefPackCompress<BinaryFile, BinaryFile>(src, dst);
					c->Compress(ei);
					delete(c);
					out_length = _ftelli64(dst.m_file);
					dst.Close(ei);

				} else {
					printf("Compressing into null ");
					RefPackCompress<BinaryFile, uint64> *c = new RefPackCompress<BinaryFile, uint64>(src, out_length);
					c->Compress(ei);
					delete(c);

				}

				t = GetTickCount() - t;

				printf("\n");
				printf(" src: %10I64u bytes\n", num_bytes);
				printf(" dst: %10I64u bytes\n", out_length);
				printf(" ratio: %I64u%%\n", out_length * 100 / num_bytes);
				printf(" time: %u.%02u secs\n\n", t / 1000, (t % 1000) / 10);
			}
		}
	}


	if (argc > 2 && (out_length == 0 || out_length >= num_bytes)) {
		src.Seek(ei, 0);
		BinaryFile dst;
		dst.Open(ei, argv[2], BinaryFile::omWrite);
		printf("Copying source file as-is.\n");
		static const uint64 tmp_size = 1024 * 1024;
		CBlobT<byte> tmp_blob;
		byte *tmp = tmp_blob.GrowSizeNC(tmp_size);
		for (uint64 remaining = num_bytes; remaining > 0;) {
			uint32 chunk_size = (uint32)MinT(tmp_size, remaining);
			src.ReadExact(ei, tmp, chunk_size);
			dst.Write(ei, tmp, chunk_size);
			remaining -= chunk_size;
		}
		dst.Close(ei);
	}

	src.Close(ei);

	if (ei.Failed()) {
		printf("Error #%d\n%s", ei.m_errno, ei.m_errmsg);
		return -2;
	}
	return 0;
}

void print_header()
{
	printf("\n");
	{
		ConColor c(FOREGROUND_GREEN | FOREGROUND_RED);
		printf("C&C3 file compressor");
		c.SetColor(FOREGROUND_GREEN);
		printf("         (C) 2007 KUDr (doctor@zl.cz). All rights reserved.\n");
		c.SetColor(FOREGROUND_GREEN | FOREGROUND_RED);
		printf("   Version 0.21     ");
		c.SetColor(FOREGROUND_GREEN);
		printf("         Free for non-commercial use. \n\n");
	}
}

void print_help()
{
	printf("Expected arguments:\n");
	printf(" 1. source file name\n");
	printf(" 2. destination file name (optional)\n");
}


