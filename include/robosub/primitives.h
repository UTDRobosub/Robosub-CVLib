#pragma once

#include "common.h"
#include "util.h"
#include <opencv2/opencv.hpp>

namespace robosub {
    class Detectable {
    protected:
        virtual double left() = 0;

        virtual double right() = 0;

        virtual double top() = 0;

        virtual double bottom() = 0;

    public:
        virtual double width() = 0;

        virtual double height() = 0;

        virtual Mat getMask(Size size) = 0;

        inline Point2d topLeft() {
            return Point2d(left(), top());
        }

        inline Point2d bottomRight() {
            return Point2d(right(), bottom());
        }


    };

    template<class T>
    class Contour_ : Detectable {

    private:
        Mat data;
        Size_<T> _boundingBox = Size_<T>();
        Point_<T> _topLeft = Point_<T>();

        int type();

        void calculateBoundingBoxDimensions() {
            if (this->_boundingBox.width != 0) return;

            //Calculate size and topLeft at the same time
            int minX = INT_MAX;
            int maxX = INT_MIN;
            int minY = INT_MAX;
            int maxY = INT_MIN;

            vector<Point_<T>> points = getPoints();
            for (Point_<T> p : points) {
                if (p.x < minX) {
                    minX = p.x;
                }
                if (p.y < minY) {
                    minY = p.y;
                }
                if (p.x > maxX) {
                    maxX = p.x;
                }
                if (p.y > maxY) {
                    maxY = p.y;
                }
            }

            this->_boundingBox = Size(maxX - minX, maxY - minY);
            this->_topLeft = Point(minX, minY);
        }


    public:
        inline Contour_(Mat &data) {
            this->data = data;
        }

        inline Mat &getMat() {
            return data;
        }

        Contour_(vector<Point_<T>> &points) {
            data = Mat(points.size(), 2, CV_32S, points.data());
        }

        int points() {
            return data.rows;
        }

        double area() {
            return abs(contourArea(data));
        }

        bool isClosed() {
            return isContourConvex(data);
        }

        Point2d centroid() {
            //C_{\mathrm x} = \frac{1}{6A}\sum_{i=0}^{n-1}(x_i+x_{i+1})(x_i\ y_{i+1} - x_{i+1}\ y_i)
            //C_{\mathrm y} = \frac{1}{6A}\sum_{i=0}^{n-1}(y_i+y_{i+1})(x_i\ y_{i+1} - x_{i+1}\ y_i)

            if (points() < 2)
                return center();

            double xSum = 0.0;
            double ySum = 0.0;
            double area = 0.0;
            vector<Point_<T>> points = getPoints();

            for (unsigned int i = 0; i < points.size() - 1; i++) {
                //cross product, (signed) double area of triangle of vertices (origin,p0,p1)
                double signedArea = (points[i].x * points[i + 1].y) - (points[i + 1].x * points[i].y);
                xSum += (points[i].x + points[i + 1].x) * signedArea;
                ySum += (points[i].y + points[i + 1].y) * signedArea;
                area += signedArea;
            }

            if (area == 0)
                return center();

            double coefficient = 3 * area;
            return Point2d(xSum / coefficient, ySum / coefficient);
        }

        Point2d center() {
            calculateBoundingBoxDimensions();
            return Point2d(_topLeft.x + (_boundingBox.width / 2), _topLeft.y + (_boundingBox.height / 2));
        }

        double height() {
            calculateBoundingBoxDimensions();
            return _boundingBox.height;
        }

        double four_sided_height() {

        }

        double width() {
            calculateBoundingBoxDimensions();
            return _boundingBox.width;
        }

        double four_sided_width() {

        }

        double top() {
            calculateBoundingBoxDimensions();
            return _topLeft.y;
        }

        double bottom() {
            calculateBoundingBoxDimensions();
            return _topLeft.y + _boundingBox.height;
        }

        double left() {
            calculateBoundingBoxDimensions();
            return _topLeft.x;
        }

        double right() {
            calculateBoundingBoxDimensions();
            return _topLeft.x + _boundingBox.width;
        }

        Rect_<T> getBoundingRect() {
            return Rect_<T>(top(), left(), width(), height());
        }

        Point_<T> topLeft() {
            calculateBoundingBoxDimensions();
            return _topLeft;
        }

        Size_<T> size() {
            calculateBoundingBoxDimensions();
            return _boundingBox;
        }

        double arcLength(bool closed) {
            return cv::arcLength(data, closed);
        }

        vector<Point_<T>> getPoints() {
            vector<Point_<T>> array;
            if (data.isContinuous()) {
                array.assign((Point_<T> *) data.datastart, (Point_<T> *) data.dataend);
            } else {
                for (int i = 0; i < data.rows; ++i) {
                    array.insert(array.end(), data.ptr<Point_<T>>(i), data.ptr<Point_<T>>(i) + data.cols);
                }
            }
            return array;
        }

        Mat getMask(Size size) {
            //returns binary mask (0 or 1)
            Mat mask = Mat::zeros(size, CV_8U);
            vector<Mat> allContours = vector<Mat>();
            allContours.emplace_back(data);
            drawContours(mask, allContours, 0, 255, cv::FILLED);
            return mask;
        }

        inline Scalar averageColor(Mat &img) {
            return mean(img, getMask(Size(img.cols, img.rows)));
        }
    };

    typedef Contour_<int> Contour;
    typedef Contour_<int> Contour2i;
    typedef Contour_<float> Contour2f;
    typedef Contour_<double> Contour2d;

    template<class T>
    class Rectangle_ : Contour_<T> {
    public:
        inline Rectangle_(Mat &data) : Contour_<T>(data) {

        }

        Rectangle_(vector<Point_<T>> &points) : Contour_<T>(points) {}

        double height() {
            vector<Point_<T>> points = Contour_<T>::getPoints();
            double side1 = Util::euclideanDistance(points.at(0).x, points.at(0).y, points.at(1).x, points.at(1).y);
            double side2 = Util::euclideanDistance(points.at(2).x, points.at(2).y, points.at(3).x, points.at(3).y);

            return (side1 + side2) / 2;
        }

        double width() {
            vector<Point_<T>> points = Contour_<T>::getPoints();
            double side1 = Util::euclideanDistance(points.at(1).x, points.at(1).y, points.at(2).x, points.at(2).y);
            double side2 = Util::euclideanDistance(points.at(0).x, points.at(0).y, points.at(3).x, points.at(3).y);

            return (side1 + side2) / 2;
        }
    };


    typedef Rectangle_<int> Rectangle;
    typedef Rectangle_<int> Rectangle2i;
    typedef Rectangle_<float> Rectangle2f;
    typedef Rectangle_<double> Rectangle2d;

    template<class T>
    class Triangle_ : Contour_<T> {
    public:
        inline Triangle_(Mat &data) : Contour_<T>(data) {

        }

        Triangle_(vector<Point_<T>> &points) : Contour_<T>(points) {}

        double height() {
            vector<Point_<T>> points = Contour_<T>::getPoints();

            double side1 = Util::euclideanDistance(points.at(0).x, points.at(0).y, points.at(1).x, points.at(1).y);
            double side2 = Util::euclideanDistance(points.at(1).x, points.at(1).y, points.at(2).x, points.at(2).y);
            double side3 = Util::euclideanDistance(points.at(2).x, points.at(2).y, points.at(0).x, points.at(0).y);
            double hypotenuse = max(max(side1, side2), side3);

            double height = 2 * this->area() / hypotenuse;

            return height;
        }

        double width() {
            vector<Point_<T>> points = Contour_<T>::getPoints();
            double side1 = Util::euclideanDistance(points.at(0).x, points.at(0).y, points.at(1).x, points.at(1).y);
            double side2 = Util::euclideanDistance(points.at(1).x, points.at(1).y, points.at(2).x, points.at(2).y);
            double side3 = Util::euclideanDistance(points.at(2).x, points.at(2).y, points.at(0).x, points.at(0).y);
            double hypotenuse = max(max(side1, side2), side3);

            return hypotenuse;
        }
    };

    typedef Triangle_<int> Triangle;
    typedef Triangle_<int> Triangle2i;
    typedef Triangle_<float> Triangle2f;
    typedef Triangle_<double> Triangle2d;

}


