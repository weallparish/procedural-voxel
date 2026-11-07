#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


inline int floorDiv(int a, int b) {
    int q = a / b;
    
    // If there is a decimal left over and a and b aren't the same sign.
    if (a % b != 0 && ((a < 0) != (b < 0))) {
        return  q - 1;
    }
    else {
        return q;
    }
}

inline int floorMod(int a, int b) {
    int m = a % b;

    // If there is a decimal left over and a and b aren't the same sign.
    if (a % b != 0 && ((a < 0) != (b < 0))) {
        return m + b;
    }
    else {
        return m;
    }
}

inline int getSign(const float f) {
    // Get furthest bit to the left in float.
    return 1 - (int)(((unsigned int&)f) >> 31) * 2;
}

inline glm::vec3 signOfDir(const glm::vec3& v) {
    return glm::vec3(getSign(v.x), getSign(v.y), getSign(v.z));
}