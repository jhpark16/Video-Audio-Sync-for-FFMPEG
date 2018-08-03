#pragma once

#include <vector>
// shell api support
#include "shlwapi.h"
#pragma comment(lib, "shlwapi.lib")

// The open file dialog tells you about multiple files selected
// by giving you a bunch of strings in a TCHAR array, each null-terminated.
// The final string is terminated by two null chars
// This class breaks that down into a directory and vector of file names
class CFileDialogFileList
{
  typedef std::vector< CString > string_vector;
  CString m_directory;
  string_vector m_filenames;
public:
  CFileDialogFileList(const TCHAR *file_spec = NULL)
  {
    if (file_spec)
      parse_file_spec(file_spec);
  }
  CFileDialogFileList &operator=(const TCHAR *file_spec)
  {
    m_filenames.clear();
    parse_file_spec(file_spec);
    return *this;
  }
  CString directory(void) { return m_directory; }
  string_vector &files(void) { return m_filenames; }
  bool files_exist(void) { return (m_filenames.size() != 0); }
  int num_files(void) { return (m_filenames.size()); }
  CString operator[] (int index)
  {
    ATLASSERT(index >= 0 && index < m_filenames.size());
    return m_filenames[index];
  }

private:
  void parse_file_spec(const TCHAR *file_spec)
  {
    ATLASSERT(file_spec != NULL);

    m_directory = file_spec;

    while (*file_spec)
      ++file_spec;
    ++file_spec; // eat the NULL
    while (*file_spec)
    {
      m_filenames.push_back(file_spec);
      while (*file_spec)
        ++file_spec;
      ++file_spec; // eat the NULL
    }
    // only 1 file
    if (m_filenames.size() == 0)
    {
      // the file name
      CString file_name = m_directory;
      ::PathStripPath(file_name.GetBuffer(file_name.GetLength()));
      file_name.ReleaseBuffer();
      m_filenames.push_back(file_name);
      // the directory
      ::PathRemoveFileSpec(m_directory.GetBuffer(m_directory.GetLength()))
        ;
      m_directory.ReleaseBuffer();
    }
    ::PathAddBackslash(m_directory.GetBuffer(m_directory.GetLength() + 1))
      ;
    m_directory.ReleaseBuffer();

  }

};
