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
const QString moodleMainDomain = "https://www1.elearning.rwth-aachen.de/";
const QString moodleMainUrl = moodleMainDomain + "pluginfile.php";
const QString moodleApiDomain = "https://moped.ecampus.rwth-aachen.de/";
const QString moodleApiUrlBase = moodleApiDomain + "proxy/api/v2/";
const QString moodleApiDocs = moodleApiUrlBase + "Documentation";
const QString moodleApiUrl = moodleApiUrlBase + "eLearning/Moodle/";
const QString moodleGetMyEnrolledCoursesUrl = moodleApiUrl + "getmyenrolledcourses";
const QString moodleGetFilesUrl = moodleApiUrl + "getfiles";
const QString moodleDownloadFileUrl = moodleApiUrl + "downloadfile";

#endif // URLS

