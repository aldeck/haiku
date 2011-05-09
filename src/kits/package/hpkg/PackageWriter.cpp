/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/PackageWriter.h>

#include <new>

#include <package/hpkg/PackageWriterImpl.h>


namespace BPackageKit {

namespace BHPKG {


BPackageWriter::BPackageWriter(BPackageWriterListener* listener)
	:
	fImpl(new (std::nothrow) PackageWriterImpl(listener))
{
}


BPackageWriter::~BPackageWriter()
{
	delete fImpl;
}


status_t
BPackageWriter::Init(const char* fileName)
{
	if (fImpl == NULL)
		return B_NO_MEMORY;

	return fImpl->Init(fileName);
}


status_t
BPackageWriter::AddEntry(const char* fileName)
{
	if (fImpl == NULL)
		return B_NO_INIT;

	return fImpl->AddEntry(fileName);
}


status_t
BPackageWriter::Finish()
{
	if (fImpl == NULL)
		return B_NO_INIT;

	return fImpl->Finish();
}


}	// namespace BHPKG

}	// namespace BPackageKit
