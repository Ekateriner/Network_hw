#pragma once
#ifndef MIPT_NETWORK_ENTITY_H
#define MIPT_NETWORK_ENTITY_H

#include <utility>
#include <cstdint>
#include <cmath>

struct Entity {
  //uint32_t entity_id;
  int32_t user_id; // -1 - AI
  std::pair<float, float> pos;
  
  float radius;
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

class BitVector {
public:
  BitVector() = default;
  
  BitVector(char* begin, char* end):
    data_vector(begin, end),
    counter(0) {
  }
    
  void push_back(bool val) {
    if (counter == 0) {
      data_vector.push_back(0);
    }
    data_vector[data_vector.size() - 1] = char(int(data_vector[data_vector.size() - 1]) | (int(val) << counter));
    counter = (counter + 1) % 8;
  }
  
  char* data() {
    return data_vector.data();
  }
  
  size_t size() {
    return data_vector.size();
  }
  
  size_t bit_size() {
    return data_vector.size() * 8 - (counter > 0) + counter;
  }
  
  bool operator[](size_t ind) {
    size_t outer_ind = ind / 8;
    size_t inner_ind = ind % 8;
    
    return bool((int(data_vector[outer_ind]) >> inner_ind) % 2);
  }
  
private:
  std::vector<char> data_vector;
  int counter = 0;
  //size_t _size = 0;
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
