#ifndef URLS
#define URLS

#include <QString>

const QString l2pMainUrl = "https://www3.elearning.rwth-aachen.de/";
const QString l2pApiUrl = l2pMainUrl + "_vti_bin/L2PServices/api.svc/v1/";
const QString viewAllCourseInfoByCurrentSemesterUrl = l2pApiUrl + "viewAllCourseInfoByCurrentSemester";
const QString viewAllCourseInfoUrl = l2pApiUrl + "viewAllCourseInfo";
const QString viewActiveFeaturesUrl = l2pApiUrl + "viewActiveFeatures";
const QString moodleMainUrl = "http://d-sp02.devlef.campus.rwth-aachen.de/";
const QString moodleApiUrl = moodleMainUrl + "ProxyService_ms/api/v2/moodle/";
const QString moodleGetMyEnrolledCourses = moodleApiUrl + "getmyenrolledcourses";
const QString moodleGetFiles = moodleApiUrl + "getfiles";

#endif // URLS

