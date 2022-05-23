#pragma once
#ifndef MIPT_NETWORK_ENTITY_H
#define MIPT_NETWORK_ENTITY_H

#include <utility>
#include <cstdint>
#include <cmath>

struct Entity {
  //uint32_t entity_id;
  int user_id; // -1 - AI
  std::pair<float, float> pos;
  
  float radius;
  int target; //for AI
  
//  Entity() {
//    entity_id = 0;
//    user_id = -1;
//    pos = std::make_pair(0, 0);
//    radius = 0;
//    target = 0;
//  }
//
//  Entity(uint32_t _id, int _user_id, std::pair<float, float> _pos, float _radius, int _target):
//    entity_id(_id),
//    user_id(_user_id),
//    pos(std::move(_pos)),
//    radius(_radius),
//    target(_target)
//  {}
};

enum Field {
  UserId,
  Position_X,
  Position_Y,
  Radius
};

union Value {
  float fl_x;
  int int_x;
};

struct DeltaEntity {
  uint32_t entity_id;
  Field field;
  Value value;
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
