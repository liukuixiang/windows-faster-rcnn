/*
 * yuStdDirFiles.h
 *
 *  Created on: Mar 3, 2016
 *      Author: qulei@Anhui University
 *      注意:该Demo SDK有效使用时间为2016年5月1日前,逾期将停止工作!
 */

#ifndef _YU_STD_DIR_FILES_H_
#define _YU_STD_DIR_FILES_H_

#include <vector>
#include "DetectionCommon.h"

using namespace std;

// Read file names of specified name extensions in the given dir.
PEOPLEDETECTION_DLL_API vector<string> yuStdDirFiles( string DirName, vector<string> FileExtensions, bool bRecursive = false );
PEOPLEDETECTION_DLL_API bool DeleteDirectory(const char* strDirName, bool bSelfDelete = false);
// Split absolute file path into director+filename+extension.
PEOPLEDETECTION_DLL_API void  yuSplitFileNames( const vector<string> &AbsoNamePaths, vector<string> &DirNames, vector<string> &FileNames, vector<string> &ExtenedNames );
// Find the common part (prefix & suffix) in a list of strings.
PEOPLEDETECTION_DLL_API string yuFindPrefix( const vector<string> &strs );
PEOPLEDETECTION_DLL_API string yuFindSuffix( const vector<string> &strs );
// if FLAG=0, return the file name without path but has extension;
// if FLAG=1, return the file name without path and extension;
// if FLAG=2, return the extension name.
PEOPLEDETECTION_DLL_API string trim_file_name( string FileName, int FLAG );
// Copy file (text or image file, etc.).
PEOPLEDETECTION_DLL_API void yuCopyFile( const char *src, const char *dst );

#endif /* _YU_STD_DIR_FILES_H_ */
