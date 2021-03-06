/****************************************************************************
* 
*  Computer Vision, Fall 2011
*  New York University
*
*  Created by Otavio Braga (obraga@cs.nyu.edu)
*
****************************************************************************/

#include <iostream>
#include <opencv2/core/core.hpp>
#include "FitEllipse.h"
#include <Eigen/Eigenvalues>
#include <Eigen/Dense>
#include <math.h>

using namespace std;

void FitEllipse(std::vector<cv::Vec2d> &pts,
                double *sx, double *sy, double *theta, cv::Vec2d *center)
{
    // !!!!!!!!!!!!!!!!!!!! Implement this !!!!!!!!!!!!!!!!!!!!
    //std::cout << "Warning: FitEllipse not implemented!\n";
	if(pts.size() == 0){
		*center = cv::Vec2d(0,0);
    	*sx = 1.0;
   	 	*sy = 1.0;
   	 	*theta = 0.0;
		return;
	}
	double x[pts.size()][6];
	cv::Mat S;
	cv::Matx33d D, E, F, G, H, H_I, G_I;
	cv::Matx33d temp;
	for(int i=0; i<pts.size(); i++){
		x[i][0] = pow(pts[i][0],2);
		x[i][1] = pts[i][0] * pts[i][1];
		x[i][2] = pow(pts[i][1],2);
		x[i][3] = pts[i][0];
		x[i][4] = pts[i][1];
		x[i][5] = 1;
	}
	cv::Mat x_t = cv::Mat (pts.size(), 6, CV_64F, x);
	S = x_t.t() * x_t;

	for(int i=0; i<3; i++){
		for(int j=0; j<3; j++){
			D(i, j) = S.at<double>(i,j);
			E(i, j) = S.at<double>(i,j+3);
			F(i, j) = S.at<double>(i+3,j);
			G(i, j) = S.at<double>(i+3,j+3);
		}
	}
	
	H(0,0) = 0;		H(0,1) = 0;		H(0,2) = 2;
	H(1,0) = 0;		H(1,1) = -1;	H(1,2) = 0;
	H(2,0) = 2;		H(2,1) = 0;		H(2,2) = 0;
	
	// H_I, G_I
	cv::invert(G,G_I);
	cv::invert(H,H_I);
	
    temp = H_I * (D - E * G_I * F);

	// change from cv_matrix to eigen_matrix
	Eigen::MatrixXf m(3,3);
	for(int i=0; i<3; i++)
		for(int j=0; j<3; j++)
			m(i, j) = temp(i,j);

	double a, b, c, d, e, f;
	Eigen::EigenSolver<Eigen::MatrixXf> eigen;
	eigen.compute(m);
	double max = eigen.eigenvalues()[0].real();
	int num=0; 
	for(int i=1; i<3; i++){
		if(eigen.eigenvalues()[i].real() > max){
			num = i;
			max = eigen.eigenvalues()[i].real();
		}
	}
	// sx, sy
	double max_x, max_y, min_x, min_y;
	max_x = min_x = pts[0][0];
	max_y = min_y = pts[0][1];
	for(int i=0; i<pts.size(); i++){
		if(pts[i][0] > max_x)
			max_x = pts[i][0];
		if(pts[i][0] < min_x)
			min_x = pts[i][0];
		if(pts[i][1] > max_y)
			max_y = pts[i][1];
		if(pts[i][1] < min_y)
			min_y = pts[i][1];
	}

	*sx = (max_x - min_x) / 2;
	*sy = (max_y - min_y) / 2;
	
	if(max <= 0){	
		*center = cv::Vec2d(0,0);
    	*sx = 1.0;
   	 	*sy = 1.0;
   	 	*theta = 0.0;
	} else {
		cv::Matx31d a_1, a_2;
		for(int i=0; i<3; i++)
			a_1(i,0) = eigen.eigenvectors().col(num)(i).real();

		a_2 = -G_I * F * a_1;

		//cv::Matx61d a;
		a = a_1(0,0);
		b = a_1(1,0);
		c = a_1(2,0);
		d = a_2(0,0);
		e = a_2(1,0);
		f = a_2(2,0);
		

		// theta
		*theta = (1/2) * atan2(b, a-c);

		// center
		double temp_u, temp_v, temp_w;
		temp_u = - (d*cos(*theta) + e*sin(*theta)) / (2*(a*pow(cos(*theta),2)+c*pow(sin(*theta),2)+b*sin(*theta)*cos(*theta)));
		temp_v = -(-d*sin(*theta) + e*cos(*theta)) / (2*(a*pow(sin(*theta),2)+c*pow(cos(*theta),2)-b*sin(*theta)*cos(*theta)));
		
		(*center)[0] = temp_u*cos(*theta) - temp_v*sin(*theta);
		(*center)[1] = temp_u*sin(*theta) + temp_v*cos(*theta);
		
		// sx, sy - re-calculate
		double temp_x, temp_y;
		temp_w = f - (a*pow(cos(*theta),2) + c * pow(sin(*theta),2) + b*sin(*theta)*cos(*theta))*pow(-(d*cos(*theta) + e*sin(*theta)) / (2*(a*pow(cos(*theta),2)+c*pow(sin(*theta),2)+b*sin(*theta)*cos(*theta))),2) - (a * pow(sin(*theta),2) + c * pow(cos(*theta),2) - b*sin(*theta)*cos(*theta)) *pow(-(-d*sin(*theta) + e * cos(*theta)) / (2*(a*pow(sin(*theta),2)+c*pow(cos(*theta),2)-b*sin(*theta)*cos(*theta))),2);

		temp_x = -temp_w
				/(a*pow(cos(*theta),2) + c*pow(sin(*theta),2) + b*sin(*theta)*cos(*theta));

		temp_y = -temp_w
				/(a*pow(sin(*theta),2) + c*pow(cos(*theta),2) - b*sin(*theta)*cos(*theta));

		if(temp_x < 0)
			temp_x = -sqrt(-temp_x);
		else
			temp_x = sqrt(temp_x);
	
		if(temp_y < 0)
			temp_y = -sqrt(-temp_y);
		else
			temp_y = sqrt(temp_y);
		
		if((*sx>*sy && temp_x<temp_y) || (*sx<*sy && temp_x>temp_y)){
			*sx = temp_y;
			*sy = temp_x;
		} else{
			*sx = temp_x;
			*sy = temp_y;
		}
	}	

}

