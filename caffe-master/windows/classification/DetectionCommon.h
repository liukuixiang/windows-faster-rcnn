/*
 * DetectionCommon.h
 *
 *  Created on: Mar 3, 2016
 *      Author: qulei@Anhui University
 *      注意:该Demo SDK有效使用时间为2016年5月1日前,逾期将停止工作!
 */

#ifndef _DETECTION_COMMON_H_
#define _DETECTION_COMMON_H_

#if defined(WIN32) && !defined(DIRECT_INCLUDE_CMN_SRC)
#ifdef PEOPLEDETECTION_LIB_EXPORTS 
#define PEOPLEDETECTION_DLL_API __declspec(dllexport) 
#else 
#define PEOPLEDETECTION_DLL_API __declspec(dllimport) 
#endif 
#else
#define PEOPLEDETECTION_DLL_API
#endif

#endif /* _DETECTION_COMMON_H_ */
