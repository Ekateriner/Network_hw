#pragma once
#ifndef MIPT_NETWORK_ENTITY_H
#define MIPT_NETWORK_ENTITY_H

#include <utility>
#include <cstdint>
#include <cmath>

struct Entity {
  uint32_t entity_id;
  int user_id; // -1 - AI
  std::pair<float, float> pos;
  
  float radius;
  int target; //for AI
};

inline float dist(const std::pair<float, float>& pos1, const std::pair<float, float>& pos2) {
  return std::sqrt((pos1.first - pos2.first) * (pos1.first - pos2.first) + (pos1.second - pos2.second) * (pos1.second - pos2.second));
}

inline float dist2(const std::pair<float, float>& pos1, const std::pair<float, float>& pos2) {
  return (pos1.first - pos2.first) * (pos1.first - pos2.first) + (pos1.second - pos2.second) * (pos1.second - pos2.second);
}

inline std::pair<float, float> operator-(const std::pair<float, float>& pos1, const std::pair<float, float>& pos2) {
  return std::make_pair(pos1.first - pos2.first, pos1.second - pos2.second);
}

inline std::pair<float, float> operator+(const std::pair<float, float>& pos1, const std::pair<float, float>& pos2) {
  return std::make_pair(pos1.first + pos2.first, pos1.second + pos2.second);
}

inline std::pair<float, float> operator/(const std::pair<float, float>& pos1, const std::pair<float, float>& pos2) {
  return std::make_pair(pos1.first / pos2.first, pos1.second / pos2.second);
}

inline std::pair<float, float> operator/(const std::pair<float, float>& pos1, const float& c) {
  return std::make_pair(pos1.first / c, pos1.second / c);
}

inline std::pair<float, float> operator*(const std::pair<float, float>& pos1, const float& c) {
  return std::make_pair(pos1.first * c, pos1.second * c);
}


#endif //MIPT_NETWORK_ENTITY_H
