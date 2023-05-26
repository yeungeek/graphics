#include <cmath>
#include <iostream>
#include <Eigen/Core>
#include <Eigen/Dense>

using namespace std;
using namespace Eigen;
int main()
{
    float a = 1.0, b = 2.0;
    cout << a << endl;
    cout << a / b << endl;
    cout << acos(-1) << endl;
    cout << "sin: " << sin(90.0 / 180 * acos(-1)) << endl;
    // Vector
    cout << "Example of vector" << endl;
    // Eigen column
    Vector3f v(1.0f, 2.0f, 3.0f);
    Vector3f w(1.0f, 0.0f, 0.0f);
    cout << "Vector output: " << endl;
    cout << v << endl;

    cout << "Vector add: " << endl;
    cout << v + w << endl;

    // Multiply
    cout << "Vector multiply" << endl;
    cout << 3.0f * v << endl;
    cout << v * 2.0f << endl;

    // Example of matrix
    Matrix3f e1, e2;
    e1 << 1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f;

    e2 << 2.0f, 3.0f, 1.0f,
        4.0f, 6.0f, 5.0f,
        9.0f, 7.0f, 8.0f;

    cout << "Example of matrix" << endl;
    cout << e1 << endl;

    cout << "In memory (column-major - default):" << endl;
    for (int i = 0; i < e1.size(); i++)
        cout << *(e1.data() + i) << "  ";
    cout << endl;
    // e1 + e2
    cout << e1 + e2 << endl;
    cout << e1 * 2 << endl;
    cout << e1 * e2 << endl;
    cout << e1 * v << endl;

    // Matrix3d R1 = Matrix3d::Identity();
    // cout << "Matrix3d: \n" << R1 << endl;
    // // origin(2,1) rotation 45, translate(1,2)
    // // rotation
    // Vector3d origin = Vector3d(2, 1, 1);
    // AngleAxisd r_V(M_PI / 4, Vector3d(0, 0, 1));
    // Matrix3d t_R = r_V.matrix();
    // cout << "Roation Matrix3d: \n" << t_R << endl;

    // R1(0, 2) = 1;
    // R1(1, 2) = 2;

    // cout << "Result: \n " << R1 * t_R * origin << endl;
    Vector3f p = Vector3f(2, 1, 1);
    cout << "origin: \n" << p << endl;
    float angle = M_PI / 4;
    Matrix3f r_M;
    r_M << 
        cos(angle), -sin(angle), 0,
        sin(angle), cos(angle), 0,
        0, 0, 1;
    cout << "rotation matrix: \n" << r_M << endl;
    Matrix3f t_M;
    t_M << 
        1,0,1,
        0,1,2,
        0,0,1;
    cout << "translation matrix: \n" << t_M << endl;
    cout << "tranform result: \n" << t_M * r_M * p << endl;
    return 0;
}