#ifndef URLS
#define URLS

#include <QString>

const QString l2pApiDomain = "https://www3.elearning.rwth-aachen.de/";
const QString l2pApiUrl = l2pApiDomain + "_vti_bin/L2PServices/api.svc/v1/";
const QString l2pApiDocs = l2pApiUrl + "Documentation";
const QString viewAllCourseInfoByCurrentSemesterUrl = l2pApiUrl + "viewAllCourseInfoByCurrentSemester";
const QString viewAllCourseInfoUrl = l2pApiUrl + "viewAllCourseInfo";
const QString viewActiveFeaturesUrl = l2pApiUrl + "viewActiveFeatures";
const QString l2pDownloadFileUrl = l2pApiUrl + "downloadFile/";
const QString moodleMainDomain = "https://demo9.elearning.rwth-aachen.de/";
const QString moodleMainUrl = moodleMainDomain + "pluginfile.php";
const QString moodleApiDomain = "http://d-sp04.devlef.campus.rwth-aachen.de/";
const QString moodleApiUrlBase = moodleApiDomain + "git_ms/ProxyService/api/v2/";
const QString moodleApiDocs = moodleApiUrlBase + "Documentation";
const QString moodleApiUrl = moodleApiUrlBase + "moodle/";
const QString moodleGetMyEnrolledCoursesUrl = moodleApiUrl + "getmyenrolledcourses";
const QString moodleGetFilesUrl = moodleApiUrl + "getfiles";
const QString moodleDownloadFileUrl = moodleApiUrl + "downloadfile";

#endif // URLS

