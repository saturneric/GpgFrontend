
#include "BuildInfoUtils.h"

#include "GpgFrontendBuildInfo.h"

namespace GpgFrontend {

auto GetProjectVersion() -> QString {
  return (QStringList{} << "v" << VERSION_MAJOR << "." << VERSION_MINOR << "."
                        << VERSION_PATCH)
      .join("");
}

auto GetProjectBuildVersion() -> QString { return BUILD_VERSION; }

auto GetProjectBuildTimestamp() -> QDateTime {
  return QDateTime::fromString(BUILD_TIMESTAMP, Qt::ISODate);
}

auto GetProjectBuildGitBranchName() -> QString { return GIT_BRANCH_NAME; }

auto GetProjectBuildGitCommitHash() -> QString { return GIT_COMMIT_HASH; }

auto GetProjectBuildGitVersion() -> QString { return GIT_VERSION; }

auto GetHttpRequestUserAgent() -> QString { return HTTP_REQUEST_USER_AGENT; }
};  // namespace GpgFrontend