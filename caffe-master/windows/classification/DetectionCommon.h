/*
 * DetectionCommon.h
 *
 *  Created on: Mar 3, 2016
 *      Author: qulei@Anhui University
 *      ע��:��Demo SDK��Чʹ��ʱ��Ϊ2016��5��1��ǰ,���ڽ�ֹͣ����!
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
