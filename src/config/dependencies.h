#ifndef DEPENDENCIES_H
#define DEPENDENCIES_H

#include "kgd/settings/configfile.h"

namespace config {

struct CONFIG_FILE(Dependencies) {
  DECLARE_CONST_PARAMETER(std::string, cxxoptsCommitHash)
  DECLARE_CONST_PARAMETER(std::string, cxxoptsCommitMsg)
  DECLARE_CONST_PARAMETER(std::string, cxxoptsCommitDate)

  DECLARE_CONST_PARAMETER(std::string, jsonCommitHash)
  DECLARE_CONST_PARAMETER(std::string, jsonCommitMsg)
  DECLARE_CONST_PARAMETER(std::string, jsonCommitDate)

  DECLARE_CONST_PARAMETER(std::string, toolsBuildDate)
  DECLARE_CONST_PARAMETER(std::string, toolsCommitHash)
  DECLARE_CONST_PARAMETER(std::string, toolsCommitMsg)
  DECLARE_CONST_PARAMETER(std::string, toolsCommitDate)

  DECLARE_CONST_PARAMETER(std::string, aptBuildDate)
  DECLARE_CONST_PARAMETER(std::string, aptCommitHash)
  DECLARE_CONST_PARAMETER(std::string, aptCommitMsg)
  DECLARE_CONST_PARAMETER(std::string, aptCommitDate)

  DECLARE_CONST_PARAMETER(std::string, reusWorldBuildDate)
  DECLARE_CONST_PARAMETER(std::string, reusWorldCommitHash)
  DECLARE_CONST_PARAMETER(std::string, reusWorldCommitMsg)
  DECLARE_CONST_PARAMETER(std::string, reusWorldCommitDate)

  static auto saveState (void) {
    return config_iterator();
  }

  static bool compareStates (const ConfigIterator &previous,
                             const std::string &constraints) {
    const ConfigIterator &current = saveState();
    std::set<ConfigIterator::key_type> keys;
    for (const auto &lhs: previous) keys.insert(lhs.first);
    for (const auto &rhs: current) keys.insert(rhs.first);

    bool ok = true;
    for (const auto &key: keys) {
      auto lhsIT = previous.find(key);
      if (lhsIT == previous.end()) {
        ok = false;
        std::cerr << "Found previously unkown key '" << key << "'" << std::endl;
        continue;
      }

      auto rhsIT = current.find(key);
      if (rhsIT == current.end()) {
        ok = false;
        std::cerr << "Lost track of key '" << key << "'" << std::endl;
        continue;
      }

      std::string lhsStr, rhsStr;
      lhsStr = static_cast<std::ostringstream&>((std::ostringstream() << lhsIT->second)).str();
      rhsStr = static_cast<std::ostringstream&>((std::ostringstream() << rhsIT->second)).str();

      if (lhsStr != rhsStr) {
        ok = false;
        std::cerr << "Mismatching values for key '" << key << "'\n"
                  << "  previous: '" << lhsStr << "'\n"
                  << "   current: '" << rhsStr << "'" << std::endl;
      }
    }

    return ok;
  }
};
}

#endif // DEPENDENCIES_H
