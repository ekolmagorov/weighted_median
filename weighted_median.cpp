#include <algorithm>
#include <clocale>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <tuple>
#include <unordered_map>
#include <vector>

using Coords = std::pair<float, float>;
using Row = std::tuple<std::string, Coords, size_t>;

//-------------------Обработка входного потока строк----------------------

//Приведение широты и долготы к одному формату - десятичному
float parseCoord(const std::string &coord) {
  float f_coord = 0;

  if (coord.find("°") != std::string::npos) {
    size_t pos_degrees = coord.find("°"), pos_minutes = coord.find("’");
    size_t offset = pos_minutes - pos_degrees - 2;
    std::string degrees = coord.substr(0, pos_degrees);

    std::string minutes = coord.substr(pos_degrees + 2, offset);
    float f_minutes = std::stof(minutes) / 60;
    f_coord = std::stof(degrees) + f_minutes;
  } else {
    f_coord = std::stof(coord);
  }

  return f_coord;
}

//Обработка одной строки
Row parseString(const std::string &info, const std::string &&delimiter) {

  std::vector<std::string> tokens;
  size_t delimiter_pos = 0;
  size_t last_pos = 0;

  while ((delimiter_pos = info.find(delimiter, last_pos)) !=
         std::string::npos) {

    std::string token = info.substr(last_pos, delimiter_pos - last_pos);
    if (token.length()) {
      tokens.push_back(token);
    }
    last_pos = delimiter_pos + 1;
  }
  int popularity = std::stoi(info.substr(last_pos, info.length() - last_pos));

  Coords c = std::make_pair(parseCoord(tokens[1]), parseCoord(tokens[2]));
  Row parsed_string = std::make_tuple(tokens[0], c, popularity);
  return parsed_string;
}

using CityInfo = std::tuple<std::pair<float, float>, float>;
//построчное чтение со стандартного потока ввода
std::unordered_map<std::string, CityInfo> getCityInfo() {
  std::unordered_map<std::string, CityInfo> cities;

  while (!std::cin.eof()) {
    std::string info;
    std::getline(std::cin, info);
    if (info.length() > 1) {
      Row parsed_city = parseString(info, "\t");
      std::string cityName = std::get<0>(parsed_city);
      std::pair<float, float> lat_long = std::get<1>(parsed_city);
      float population = std::get<2>(parsed_city);

      cities[cityName] = std::make_tuple(lat_long, population);
    }
  }
  return cities;
}

void printCities(std::unordered_map<std::string, CityInfo> &cities) {
  for (auto &r : cities) {

    std::cout << r.first << " (" << std::get<0>(r.second).first << ","
              << std::get<0>(r.second).second << ") " << std::get<1>(r.second)
              << std::endl;
  }
  return;
}
//-----------------------------------------------------------------------------

//--------------------Блок вычисления оптимального города----------------------

template <class T> void merge(std::vector<T> &v, int p, int q, int r) {
  int size1 = q - p + 1;
  int size2 = r - q;
  std::vector<T> L(size1);
  std::vector<T> R(size2);

  for (int i = 0; i < size1; i++) {
    L[i] = v[p + i];
  }
  for (int j = 0; j < size2; j++) {
    R[j] = v[q + j + 1];
  }

  int i = 0, j = 0;
  int k;
  for (k = p; k <= r && i < size1 && j < size2; k++) {
    if (std::get<1>(L[i]) < std::get<1>(R[j])) {
      v[k] = L[i];
      i++;
    } else {
      v[k] = R[j];
      j++;
    }
  }
  for (i = i; i < size1; ++i) {
    v[k] = L[i];
    k++;
  }

  for (j = j; j < size2; j++) {
    v[k] = R[j];
    k++;
  }
}

template <class T> void merge_sort(std::vector<T> &v, int p, int r) {
  if (p < r) {
    int q = (p + r) / 2;
    merge_sort(v, p, q);
    merge_sort(v, q + 1, r);
    merge(v, p, q, r);
  }
}

std::unordered_map<std::string, CityInfo>
filter(const std::unordered_map<std::string, CityInfo> &cities,
       const float min_val) {

  std::unordered_map<std::string, CityInfo> filtered_cities;
  for (auto &elem : cities) {
    if (std::get<1>(elem.second) > min_val)
      filtered_cities[elem.first] = elem.second;
  }

  return filtered_cities;
}
//

using c_w = std::pair<std::string, float>;
using c_c_w = std::tuple<std::string, float, float>;

std::unordered_map<std::string, float> sort_coord(std::vector<c_c_w> &vals) {

  merge_sort(vals, 0, vals.size() - 1);

  std::vector<float> cum_coord_weight = {0};
  std::vector<float> cum_weight = {0};
  std::unordered_map<std::string, float> result;
  float total_weight = 0, total_coord_weight = 0;
  for (c_c_w &elem : vals) {
    total_weight += std::get<2>(elem);
    total_coord_weight += std::get<1>(elem) * std::get<2>(elem);
    cum_coord_weight.push_back(total_coord_weight);
    cum_weight.push_back(total_weight);
  }

  for (size_t i = 0; i < vals.size(); i++) {
    float residual_weight = total_weight - cum_weight[i];
    float residual_coord_weight = total_coord_weight - cum_coord_weight[i];

    float first_part = std::get<1>(vals[i]) * (cum_weight[i] - residual_weight);
    float second_part = residual_coord_weight - cum_coord_weight[i];
    float weight = first_part + second_part;
    result[std::get<0>(vals[i])] = weight;
  }

  return result;
}

void create_sorted(std::unordered_map<std::string, CityInfo> &vals) {
  std::vector<c_c_w> longitude_tuples, latitude_tuples;
  for (auto &r : vals) {
    std::string name = r.first;
    CityInfo ci = r.second;
    float latitude = std::get<0>(ci).first, longitude = std::get<0>(ci).second;
    float population = std::get<1>(ci);

    c_c_w longitude_tuple = std::make_tuple(name, longitude, 2 * population);
    c_c_w latitude_tuple = std::make_tuple(name, latitude, population);

    longitude_tuples.push_back(longitude_tuple);
    latitude_tuples.push_back(latitude_tuple);
  }

  std::unordered_map<std::string, float> longitude_vals =
      sort_coord(longitude_tuples);

  std::unordered_map<std::string, float> latitude_vals =
      sort_coord(latitude_tuples);

  std::vector<c_w> final_weights;
  for (auto &it : longitude_vals) {
    std::string city = it.first;
    float weight = it.second + latitude_vals[city];
    final_weights.push_back(std::make_pair(city, weight));
  }

  merge_sort(final_weights, 0, final_weights.size() - 1);
  for (auto &elem : final_weights) {

    std::string name = elem.first;
    float latitude = std::get<0>(vals[name]).first;
    float longitude = std::get<0>(vals[name]).second;
    float population = std::get<1>(vals[name]);
    printf("%s,%.2f, %.2f, %.0f ,%.2f \n", name.c_str(), latitude, longitude,
           population, elem.second);
  }
}
//-----------------------------------------------------------------------------

int main(int argc, char *argv[]) {

  setlocale(LC_CTYPE, "rus");
  std::unordered_map<std::string, CityInfo> cities = getCityInfo();

  float min_population = 0;
  if (argc > 1) {
    min_population = std::stoi(argv[1]);
  }

  cities = filter(cities, min_population);
  // printCities(cities);
  create_sorted(cities);
  return 0;
}
