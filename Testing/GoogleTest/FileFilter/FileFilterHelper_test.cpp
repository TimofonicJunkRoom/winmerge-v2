#include <gtest/gtest.h>
#include <windows.h>
#include <tchar.h>
#include <vector>
#include "FileFilterHelper.h"
#include "Environment.h"
#include "paths.h"

namespace
{
	// The fixture for testing string differencing functions.
	class FileFilterHelperTest : public testing::Test
	{
	protected:
		// You can remove any or all of the following functions if its body
		// is	empty.

		FileFilterHelperTest()
		{
			// You can do set-up work for each test	here.
		}

		virtual ~FileFilterHelperTest()
		{
			// You can do clean-up work	that doesn't throw exceptions here.
		}

		// If	the	constructor	and	destructor are not enough for setting up
		// and cleaning up each test, you can define the following methods:

		virtual void SetUp()
		{
			// Code	here will be called	immediately	after the constructor (right
			// before each test).
			TCHAR temp[MAX_PATH] = {0};
			GetModuleFileName(NULL, temp, MAX_PATH);
			env_SetProgPath(paths_GetPathOnly(temp) + _T("/../../FileFilter"));
			m_fileFilterHelper.LoadAllFileFilters();
		}

		virtual void TearDown()
		{
			// Code	here will be called	immediately	after each test	(right
			// before the destructor).
		}

		// Objects declared here can be used by all tests in the test case for Foo.
		FileFilterHelper m_fileFilterHelper;
	};

	TEST_F(FileFilterHelperTest, GetManager)
	{
		EXPECT_TRUE(m_fileFilterHelper.GetManager() != NULL);
	}

	TEST_F(FileFilterHelperTest, GetFileFilterPath)
	{
		String filterpath, filtername;
		filterpath = m_fileFilterHelper.GetFileFilterPath(_T("simple include file"));
		EXPECT_TRUE(filterpath.find_first_of(_T("Filters\\simple_include_file.flt")) != String::npos);
		filtername = m_fileFilterHelper.GetFileFilterName(filterpath.c_str());
		EXPECT_TRUE(filtername.compare(_T("simple include file")) == 0);

		filterpath = m_fileFilterHelper.GetFileFilterPath(_T("simple include dir"));
		EXPECT_TRUE(filterpath.find_first_of(_T("Filters\\simple_include_dir.flt")) != String::npos);
		filtername = m_fileFilterHelper.GetFileFilterName(filterpath.c_str());
		EXPECT_TRUE(filtername.compare(_T("simple include dir")) == 0);

		filterpath = m_fileFilterHelper.GetFileFilterPath(_T("non-existent file filter name"));
		EXPECT_TRUE(filterpath.empty());

		filtername = m_fileFilterHelper.GetFileFilterName(_T("non-existent file filter path"));
		EXPECT_TRUE(filtername.empty());
	}

	TEST_F(FileFilterHelperTest, SetFileFilterPath)
	{
		std::vector<FileFilterInfo> filters;
		String selected;
		m_fileFilterHelper.SetFileFilterPath(_T(""));
		m_fileFilterHelper.GetFileFilters(&filters, selected);
		EXPECT_TRUE(selected.compare(_T("")) == 0);

		m_fileFilterHelper.SetFileFilterPath(_T("non-existent file filter path"));
		m_fileFilterHelper.GetFileFilters(&filters, selected);
		EXPECT_TRUE(selected.compare(_T("")) == 0);

		m_fileFilterHelper.SetFileFilterPath(m_fileFilterHelper.GetFileFilterPath(_T("simple include file")).c_str());
		m_fileFilterHelper.GetFileFilters(&filters, selected);
		EXPECT_TRUE(selected.find_first_of(_T("Filters\\simple_include_file.flt")) == 0);
	}

	TEST_F(FileFilterHelperTest, GetFileFilters)
	{
		std::vector<FileFilterInfo> filters;
		String selected;
		m_fileFilterHelper.GetFileFilters(&filters, selected);

		for (std::vector<FileFilterInfo>::iterator it = filters.begin(); it != filters.end(); it++)
		{
			if ((*it).name.compare(_T("simple include file")) == 0)
			{
				EXPECT_TRUE((*it).fullpath.find_first_of(_T("Filters\\simple_include_file.flt")) == 0);
				EXPECT_TRUE((*it).description.compare(_T("simple file filter long description")) == 0);
			}
			else if ((*it).name.compare(_T("simple include dir")) == 0)
			{
				EXPECT_TRUE((*it).fullpath.find_first_of(_T("Filters\\simple_include_dir.flt")) == 0);
				EXPECT_TRUE((*it).description.compare(_T("simple directory filter long description")) == 0);
			}
			else
			{
				EXPECT_TRUE(false);
			}
		}
	}

	TEST_F(FileFilterHelperTest, SetFilter)
	{
		m_fileFilterHelper.SetFilter(_T("simple include file"));
		EXPECT_EQ(m_fileFilterHelper.IsUsingMask(), FALSE);

		EXPECT_EQ(m_fileFilterHelper.includeFile(_T("a.c")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeFile(_T("a.cpp")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeFile(_T("a.ext")), FALSE);
		EXPECT_EQ(m_fileFilterHelper.includeFile(_T("")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeDir(_T("")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeDir(_T("svn")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeDir(_T("a.ext")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeDir(_T("a.b.c")), TRUE);

		m_fileFilterHelper.SetFilter(_T("simple include dir"));

		EXPECT_EQ(m_fileFilterHelper.includeFile(_T("a.c")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeFile(_T("a.cpp")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeFile(_T("a.ext")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeFile(_T("")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeDir(_T("")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeDir(_T("svn")), FALSE);
		EXPECT_EQ(m_fileFilterHelper.includeDir(_T("a.ext")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeDir(_T("a.b.c")), TRUE);

	}

	TEST_F(FileFilterHelperTest, SetMask)
	{
		m_fileFilterHelper.UseMask(TRUE);
		EXPECT_EQ(m_fileFilterHelper.IsUsingMask(), TRUE);

		m_fileFilterHelper.SetMask(_T(""));
		EXPECT_EQ(m_fileFilterHelper.includeFile(_T("a.c")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeFile(_T("a.cpp")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeFile(_T("a")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeFile(_T("")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeDir(_T("")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeDir(_T("svn")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeDir(_T("a.b")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeDir(_T("a.b.c")), TRUE);

		m_fileFilterHelper.SetMask(_T("*.c"));
		EXPECT_EQ(m_fileFilterHelper.includeFile(_T("a.c")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeFile(_T("a.cpp")), FALSE);
		EXPECT_EQ(m_fileFilterHelper.includeFile(_T("a.ext")), FALSE);
		EXPECT_EQ(m_fileFilterHelper.includeDir(_T("")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeDir(_T("a")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeDir(_T("a.b")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeDir(_T("a.b.c")), TRUE);

		m_fileFilterHelper.SetMask(_T("*.c;*.cpp;*.cxx"));
		EXPECT_EQ(m_fileFilterHelper.IsUsingMask(), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeFile(_T("a.c")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeFile(_T("a.cpp")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeFile(_T("a.cxx")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeFile(_T(".cpp")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeFile(_T("cpp")), FALSE);
		EXPECT_EQ(m_fileFilterHelper.includeFile(_T("a.cx")), FALSE);
		EXPECT_EQ(m_fileFilterHelper.includeFile(_T("a.cpp.h")), FALSE);
		EXPECT_EQ(m_fileFilterHelper.includeDir(_T("")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeDir(_T("svn")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeDir(_T("a.b")), TRUE);
		EXPECT_EQ(m_fileFilterHelper.includeDir(_T("a.b.c")), TRUE);
	}



}  // namespace