#ifndef _HTTP_H_
#define _HTTP_H_

#include <gctypes.h>
int downloadFileToBuffer(char * url, char ** buffer, wchar_t * sCurrentInfoText, bool &bForceCancel, f32 &fProgressPercentage);
bool downloadFileToDisk(char * url, char * out, wchar_t * sCurrentInfoText, bool &bForceCancel, f32 &fProgressPercentage);


#endif /* _HTTP_H_ */