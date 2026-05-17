#pragma once
// Function prototypes from matrixutils.cpp:
void BuildPerspProjMat(double *m, double fov, double aspect, double znear, double zfar);
void BuildIdentityMat(double *m);
void Translate(GLdouble *m, GLdouble x, GLdouble y, GLdouble z);
void Multiply(GLdouble *mOut, GLdouble *m1, GLdouble *m2);
void ApplyYaw(GLdouble *m, GLdouble yaw);
void EulerMatrix(double m[16], double yaw, double pitch, double roll);