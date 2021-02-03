#ifndef RMP_PATHS_HPP
#define RMP_PATHS_HPP

#include <string>

#include "puzzles.hpp"

namespace paths {

const std::string PUZZLE_DATA = "/opt/etc/puzzles";

inline std::string game_basename(const game *g)
{
    std::string ret;
    for (const char *c = g->name; *c != '\0'; c++)
        if (!isspace(*c))
            ret.push_back(tolower(*c));
    return ret;
}

inline std::string game_colors(const game *g)
{
    return PUZZLE_DATA + "/" + game_basename(g) + "_colors.conf";
}

} // namespace

#endif // RMP_PATHS_HPP