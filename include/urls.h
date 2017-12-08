#ifndef URLS
#define URLS

#include <QString>

const QString mainUrl = "https://www3.elearning.rwth-aachen.de/";
const QString apiUrl = mainUrl + "_vti_bin/L2PServices/api.svc/v1/";
const QString viewAllCourseInfoByCurrentSemesterUrl = apiUrl + "viewAllCourseInfoByCurrentSemester";
const QString viewAllCourseInfoUrl = apiUrl + "viewAllCourseInfo";
const QString viewActiveFeaturesUrl = apiUrl + "viewActiveFeatures";

#endif // URLS

