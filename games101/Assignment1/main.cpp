#include "Triangle.hpp"
#include "rasterizer.hpp"
#include <eigen3/Eigen/Eigen>
#include <iostream>
#include <opencv2/opencv.hpp>

constexpr double MY_PI = 3.1415926;

Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos)
{
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();

    Eigen::Matrix4f translate;
    translate << 1, 0, 0, -eye_pos[0],
        0, 1, 0, -eye_pos[1],
        0, 0, 1, -eye_pos[2],
        0, 0, 0, 1;

    view = translate * view;

    return view;
}

Eigen::Matrix4f get_model_matrix(float rotation_angle)
{
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();

    // TODO: Implement this function
    // Create the model matrix for rotating the triangle around the Z axis.
    // Then return it.
    //
    float angle = rotation_angle * MY_PI / 180;

    // x axis
    // model << 
    //     1, 0, 0, 0,
    //     0, cos(angle), -sin(angle), 0,
    //     0, sin(angle), cos(angle), 0,
    //     0, 0, 0, 1;

    // y axis
    // model << 
    //     cos(angle), 0, sin(angle), 0,
    //     0, 1, 0, 0,
    //     -sin(angle), 0, cos(angle), 0,
    //     0, 0, 0, 1;
    // z axis
    model << 
        cos(angle), -sin(angle), 0, 0,
        sin(angle), cos(angle), 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1;

    return model;
}

Eigen::Matrix4f get_rotation(Vector3f axis, float rotation_angle)
{
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();

    float angle = rotation_angle * MY_PI / 180;
    float nx = axis[0];
    float ny = axis[1];
    float nz = axis[2];

    Eigen::Matrix3f N;
    N << 
        0, -nx, ny,
        nz, 0, -nx,
        -ny,nx, 0;
    Eigen::Matrix3f I = Eigen::Matrix3f::Identity();
    Eigen::Matrix3f R = cos(angle) * I + (1 - cos(angle)) * axis * axis.transpose() + sin(angle) * N;

    model <<
        R(0,0),R(0,1),R(0,2),0,
        R(1,0),R(1,1),R(1,2),0,
        R(2,0),R(2,1),R(2,2),0,
        0, 0, 0, 1;
    return model;
}


Eigen::Matrix4f get_projection_matrix(float eye_fov, float aspect_ratio,
                                      float zNear, float zFar)
{
    // Students will implement this function

    Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();

    // TODO: Implement this function
    // Create the projection matrix for the given parameters.
    // Then return it.
    Eigen::Matrix4f M_trans;
    Eigen::Matrix4f M_persp;
    Eigen::Matrix4f M_ortho;
    
    float angle = 0.5 * eye_fov * M_PI / 180.0f;
    // zFar - 
    M_persp << 
               zNear, 0, 0, 0,
               0, zNear, 0, 0,
               0, 0, zNear + zFar, -zNear*zFar,
               0, 0, 1, 0;
               
    float yTop = -zNear * tan(angle);
    float yBottom = -yTop;
    float xRight = yTop * aspect_ratio;
    float xLeft = - xRight;

    M_trans <<
        1, 0, 0, -(xLeft + xRight) / 2,
        0, 1, 0, -(yTop + yBottom) / 2,
        0, 0, 1, -(zNear + zFar) / 2,
        0, 0, 0, 1;
    M_ortho <<
        2 / (xRight - xLeft), 0, 0, 0,
        0, 2 / (yTop - yBottom), 0, 0,
        0, 0, 2 / (zNear - zFar), 0,
        0, 0, 0, 1;

    M_ortho = M_ortho * M_trans;
    projection = M_ortho * M_persp * projection;

    return projection;
}

int main(int argc, const char **argv)
{
    float angle = 0;
    bool command_line = false;
    std::string filename = "output.png";

    if (argc >= 3)
    {
        command_line = true;
        angle = std::stof(argv[2]); // -r by default
        if (argc == 4)
        {
            filename = std::string(argv[3]);
        }
    }

    rst::rasterizer r(700, 700);

    Eigen::Vector3f eye_pos = {0, 0, 5};

    std::vector<Eigen::Vector3f> pos{{2, 0, -2}, {0, 2, -2}, {-2, 0, -2}};

    std::vector<Eigen::Vector3i> ind{{0, 1, 2}};

    auto pos_id = r.load_positions(pos);
    auto ind_id = r.load_indices(ind);

    int key = 0;
    int frame_count = 0;

    if (command_line)
    {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        // r.set_model(get_model_matrix(angle));
        Vector3f axis = {10,5,-2};
        r.set_model(get_rotation(axis,angle));

        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);

        cv::imwrite(filename, image);

        return 0;
    }

    while (key != 27)
    {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);

        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::imshow("image", image);
        key = cv::waitKey(10);

        std::cout << "frame count: " << frame_count++ << ", key=" << key <<'\n';

        if (key == 'a')
        {
            angle += 10;
        }
        else if (key == 'd')
        {
            angle -= 10;
        }
    }

    return 0;
}
