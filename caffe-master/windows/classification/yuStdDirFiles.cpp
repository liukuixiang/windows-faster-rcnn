#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <fstream>
#if (__GNUC__ > 2)
#include <dirent.h>

char *strlwr(char *s)
{
 while(*s != '\0')
 {
  if(*s >= 'A' && *s <= 'Z') {
     *s += 'a'-'A';
 }
 s++;
 }
 return s;
}

#else
#include <io.h>
#include <windows.h>
#endif
#include <algorithm>
#include <string.h>
#include <stdexcept>
using namespace std;

#include "yuStdDirFiles.h"

// some else directory manipulations with standard c library:
// _mkdir( const char * );
// remove( const char * );

///////////////////////////////////////////
PEOPLEDETECTION_DLL_API vector<string> yuStdDirFiles( string DirName, vector<string> FileExtensions, bool bRecursive )
// Read file names of specified name-extensions  in the given dir
// DirName can be: "C:\\WINDOWS" or "C:\\WINDOWS\\" and such like
// "." means current dir; "../" means the upper dir.
{
	set<string> Extensions;
	bool READ_ALL = FileExtensions.empty();
	if( !READ_ALL ) {
		for (int i = 0; i < FileExtensions.size(); i++) {
			transform(FileExtensions[i].begin(), FileExtensions[i].end(), FileExtensions[i].begin(), ::tolower);
		}
		Extensions.insert( FileExtensions.begin(), FileExtensions.end() );
	}

	char last = DirName[DirName.length()-1];
	if (last != '\\' && last != '/')
		DirName += '/';
	vector<string> FileNames;
	cout<<"Finding files in \""<<DirName<<"\":"<<endl;

#if (__GNUC__ > 2)

#define FILENAME_LEN 64
#define FILES_NUM 8192
    DIR *dp = opendir(DirName.c_str());
    if(dp == NULL) {
        fprintf(stderr,"cannot open directory: %s\n", DirName.c_str());
        return FileNames;
    }
    struct dirent *entry;
    char* fns, *fns2;
    fns = (char*) malloc(FILENAME_LEN * FILES_NUM);
    fns2 = (char*) malloc(FILENAME_LEN * FILES_NUM);
    int num = 0, idx;
    while((entry = readdir(dp)) != NULL) {
    	if (entry->d_type & DT_DIR) {
    		if (bRecursive && (strcmp(".", entry->d_name) != 0 && strcmp("..", entry->d_name) != 0)) {
    			vector <string> fnVec = yuStdDirFiles(DirName + entry->d_name, FileExtensions);
    			FileNames.insert(FileNames.end(), fnVec.begin(), fnVec.end());
    		}

    	} else {
        	int len = strlen(entry->d_name);
			strlwr(entry->d_name + len-4);
        	if (len > 4 && Extensions.find(entry->d_name + len-4)!=Extensions.end()) {
        		for (idx = 0; idx < num; idx++) {
        			if (strcmp(entry->d_name, fns + idx * FILENAME_LEN) < 0) {
        				break;
        			}
        		}
        		if (idx < num) {
        			memcpy(fns2 + (idx+1) * FILENAME_LEN, fns + idx * FILENAME_LEN, (num - idx) * FILENAME_LEN);
        			memcpy(fns + (idx+1) * FILENAME_LEN, fns2 + (idx+1) * FILENAME_LEN, (num - idx) * FILENAME_LEN);
        		}
        		strcpy(fns + idx * FILENAME_LEN, entry->d_name);
        		num++;
        		if (num >= FILES_NUM)
        			break;
        	}
    	}
    }
    free(fns2);
    closedir(dp);
	for (idx = 0; idx < num; idx++) {
		FileNames.push_back(DirName + (fns + idx * FILENAME_LEN));
	}
    free(fns);

#else
    _finddata_t file;
	string DirName2 = DirName + "*.*";
	intptr_t lf = _findfirst(DirName2.c_str(), &file);
	if( lf==-1l ){
		cout<<"ERROR: INCORRECT DIRNAME: "<<DirName<<endl;
		throw runtime_error("ERROR: INCORRECT DIRNAME!");
	}
	char FullPath[512];
	_fullpath( FullPath, DirName.c_str(), 512 );
	string DirPath( FullPath );

	while( _findnext( lf, &file ) == 0 ){
		if( file.attrib ==_A_SUBDIR) { // 子目录
    		if (bRecursive && (strcmp(".", file.name) != 0 && strcmp("..", file.name) != 0)) {
    			vector <string> fnVec = yuStdDirFiles(DirPath + file.name, FileExtensions);
    			FileNames.insert(FileNames.end(), fnVec.begin(), fnVec.end());
    		}

		//} else if (file.attrib !=_A_SYSTEM || file.attrib !=_A_HIDDEN) { // 系统文件、隐藏文件
		//	continue;
		} else {
			string Name = file.name;
			string ExtenedName = trim_file_name( Name, 2 );
			transform(ExtenedName.begin(), ExtenedName.end(), ExtenedName.begin(), ::tolower);
			if( Extensions.find(ExtenedName)==Extensions.end() )
				continue;
			Name = DirPath + Name;
			FileNames.push_back( Name );
		}
	}
	_findclose(lf);
#endif

    cout<<"Have found "<<FileNames.size()<<" files (with given name extensions):"<<endl;
	int min_num = std::min<int>( FileNames.size(), 5u );
	for( int i=0; i<min_num; i++ )
		cout<<"   "<<trim_file_name(FileNames[i],0)<<endl;
	cout<<"-------------- DONE --------------"<<endl;
	return FileNames;
}

//删除指定目录及其中的所有文件
PEOPLEDETECTION_DLL_API bool DeleteDirectory(const char* strDirName, bool bSelfDelete)
{
	string DirName = strDirName;
	char last = DirName[DirName.length()-1];
	if (last != '\\' && last != '/')
		DirName += '/';

#if (__GNUC__ > 2)
    DIR *dp = opendir(strDirName);
    if(dp == NULL) {
        fprintf(stderr,"cannot open directory: %s\n", strDirName);
        return false;
    }
    struct dirent *entry;
    char path[260];
    while((entry = readdir(dp)) != NULL) {
    	if (entry->d_type & DT_DIR) {
    		if (strcmp(".", entry->d_name) != 0 && strcmp("..", entry->d_name) != 0) {
    			sprintf(path,"%s%s", DirName.c_str(), entry->d_name);
    			if (!DeleteDirectory(path, true)) {
    				closedir(dp);
    				return false;
    			}
    		}

    	} else {
    		sprintf(path,"%s%s", DirName.c_str(), entry->d_name);
			if (!remove(path)) {
				closedir(dp);
				return false;
			}
    	}
    }
    closedir(dp);
    if (bSelfDelete && !remove(strDirName)) return false;

#else
    _finddata_t file;
	string DirName2 = DirName + "*.*";
	intptr_t lf = _findfirst(DirName2.c_str(), &file);
	if( lf==-1l ){
		cout<<"ERROR: INCORRECT DIRNAME: "<<DirName<<endl;
		throw runtime_error("ERROR: INCORRECT DIRNAME!");
	}
	char FullPath[512];
	_fullpath( FullPath, DirName.c_str(), 512 );
	string DirPath( FullPath );

	while( _findnext( lf, &file ) == 0 ){
		if( file.attrib ==_A_SUBDIR) { // 子目录
    		if (strcmp(".", file.name) != 0 && strcmp("..", file.name) != 0) {
				DirName = DirPath + file.name;
				DeleteDirectory(DirName.c_str(), true);
    		}

		} else {
			DirName = DirPath + file.name;
			DeleteFile(DirName.c_str());
		}
	}
	_findclose(lf);

	if (bSelfDelete && !RemoveDirectory(strDirName)) return false;
#endif

	return true;
}

///////////////////////////////////////////
// A small but useful function to modify a image file name to better display
// FLAG=0: return the file name without path, but has extension; 
// FLAG=1: return the file name without path and extension;
// FLAG=2: return the extension name.
PEOPLEDETECTION_DLL_API string trim_file_name( string FileName, int FLAG )
{
	string::size_type p1 = FileName.rfind('/');
	string::size_type p2 = FileName.find('.');
	bool flag1 = p1==string::npos;
	bool flag2 = p2==string::npos;
	string ret = FileName;
	switch( FLAG ){
	case 0:
		if( !flag1 )
			ret = FileName.substr( p1+1 );
		break;
	case 1:
		if( !flag1 && !flag2 )
			ret = FileName.substr( p1+1, p2-p1 );
		else if( !flag1 )
			ret = FileName.substr( p1+1 );
		else if( !flag2 )
			ret = FileName.substr( 0, p2 );
		break;
	case 2:
		if( !flag2 )
			ret = FileName.substr( p2 );
		else
			ret.clear();
		break;
	default:
		throw runtime_error("ERROR: INVALID FLAG!");
	}
	return ret;
}
///////////////////////////////////////////
// str1 & str2 has a common prefix whose length is the return value
unsigned findPrefix( const string &str1, const string &str2 )
{
	unsigned i = 0;
	unsigned total = std::min<size_t>( str1.size(), str2.size() );
	for( i=0; i<total; i++ )
		if( str1[i]!=str2[i] )
			break;
	return i;
}
///////////////////////////////////////////
// str1 & str2 has a common suffix whose length is the return value
unsigned findSufffix( const string &str1, const string &str2 )
{
	unsigned i = 0;
	unsigned total = std::min<size_t>( str1.size(), str2.size() );
	for( i=0; i<total; i++ ){
		unsigned j = total - i - 1;
		if( str1[j]!=str2[j] )
			break;
	}
	return i;
}
///////////////////////////////////////////
PEOPLEDETECTION_DLL_API void  yuSplitFileNames( const vector<string> &AbsoNamePaths, vector<string> &DirNames, vector<string> &FileNames, vector<string> &ExtenedNames )
// AbsoNamePaths is a list of names with absolute file path. Function will split it into dir, file_name, extension
// e.g. if AbsoNamePaths = "C:/Program Files/haha.c", then DirNames = "C:/Program Files/", FileNames = "haha", ExtenedNames = ".c"
{
	int num = AbsoNamePaths.size();
	DirNames.resize( num );
	FileNames = ExtenedNames = DirNames;
	for( int i=0; i<num; i++ ){
		string Name = AbsoNamePaths[i];
		string::size_type p = Name.rfind('/');
		if( p!=string::npos ){
			DirNames[i] = Name.substr(0,p+1);
			Name.erase(0,p+1);
		}
		p = Name.rfind('.');
		if( p!=string::npos ){
			ExtenedNames[i] = Name.substr(p);
			Name.erase(p);
		}
		FileNames[i] = Name;
	}
}
///////////////////////////////////////////
// find the common prefix of a list of strings
PEOPLEDETECTION_DLL_API string yuFindPrefix( const vector<string> &strs )
{
	string Prefix = strs[0];
	for( unsigned i=0; i<strs.size(); i++ ){
		unsigned a = findPrefix( Prefix, strs[i] );
		if( a==0 ){
			Prefix.clear();
			break;
		}
		if( a==Prefix.size() )
			continue;
		Prefix = Prefix.substr( 0, a );
	}
	return Prefix;
}
///////////////////////////////////////////
// find the common suffix of a list of strings
PEOPLEDETECTION_DLL_API string yuFindSuffix( const vector<string> &strs )
{
	string Suffix = strs[0];
	for( unsigned i=0; i<strs.size(); i++ ){
		unsigned a = findSufffix( Suffix, strs[i] );
		if( a==0 ){
			Suffix.clear();
			break;
		}
		if( a==Suffix.size() )
			continue;
		Suffix = Suffix.substr( Suffix.size()-a, a );
	}
	return Suffix;
}
///////////////////////////////////////////
// copy file
PEOPLEDETECTION_DLL_API void yuCopyFile( const char *src, const char *dst )
{
	int buf[1024];
	FILE *pr,*pw;
	pr = fopen( src, "rb" );
	pw = fopen( dst, "wb" );
	while(!feof(pr)){
		fread(buf,1,1,pr);
		fwrite(buf,1,1,pw);}
	fclose(pr);
	fclose(pw);
}
