/*
 * @Author: DI JUNKUN
 * @Date: 2025-11-11
 * Copyright (c) 2025 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _VERSION_CHECKER_H_
#define _VERSION_CHECKER_H_

#include <httplib.h>

#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>

using json = nlohmann::json;

namespace crossdesk {

std::string ExtractNumericPart(const std::string& ver) {
  size_t start = 0;
  while (start < ver.size() && !std::isdigit(ver[start])) start++;
  size_t end = start;
  while (end < ver.size() && (std::isdigit(ver[end]) || ver[end] == '.')) end++;
  return ver.substr(start, end - start);
}

std::vector<int> SplitVersion(const std::string& ver) {
  std::vector<int> nums;
  std::istringstream ss(ver);
  std::string token;
  while (std::getline(ss, token, '.')) {
    try {
      nums.push_back(std::stoi(token));
    } catch (...) {
      nums.push_back(0);
    }
  }
  return nums;
}

bool IsNewerVersion(const std::string& current, const std::string& latest) {
  auto v1 = SplitVersion(ExtractNumericPart(current));
  auto v2 = SplitVersion(ExtractNumericPart(latest));

  size_t len = std::max(v1.size(), v2.size());
  v1.resize(len, 0);
  v2.resize(len, 0);

  for (size_t i = 0; i < len; ++i) {
    if (v2[i] > v1[i]) return true;
    if (v2[i] < v1[i]) return false;
  }
  return false;
}

std::string CheckUpdate() {
  httplib::Client cli("https://version.crossdesk.cn");

  cli.set_connection_timeout(5);
  cli.set_read_timeout(5);

  if (auto res = cli.Get("/version.json")) {
    if (res->status == 200) {
      try {
        auto j = json::parse(res->body);
        std::string latest = j["latest_version"];
        return latest;
      } catch (std::exception&) {
        return "";
      }
    } else {
      return "";
    }
  } else {
    return "";
  }
}
}  // namespace crossdesk
#endif