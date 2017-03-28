/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
/*
 * With Grateful Acknowledgements to the books writen by Andre LaMothe:
 * <Tricks of the 3D Game Programming Gurus-Advanced 3D Graphics and Rasterization>
 * <Tricks of the Windows Game Programming Gurus>
 */
#include "math3d.h"

float my_cos_look[361]; // 1 extra element so we can store 0-360 inclusive
float my_sin_look[361];

//////////////////////////////////////////////////////////////

float VECTOR3D_Length(VECTOR3D_PTR va) {
  // computes the magnitude of a vector, slow

  return ((float) sqrtf(va->x * va->x + va->y * va->y + va->z * va->z));

} // end VECTOR3D_Length

///////////////////////////////////////////////////////////////

void VECTOR3D_Normalize(VECTOR3D_PTR va, VECTOR3D_PTR vn) {
  // normalizes the sent vector and returns the result in vn

  VECTOR3D_ZERO(vn);

  // compute length
  float length = VECTOR3D_Length(va);

  // test for zero length vector
  // if found return zero vector
  if (length < EPSILON_E5)
    return;

  float length_inv = 1.0 / length;

  // compute normalized version of vector
  vn->x = va->x * length_inv;
  vn->y = va->y * length_inv;
  vn->z = va->z * length_inv;

} // end VECTOR3D_Normalize

///////////////////////////////////////////////////////////////////////////////

float Fast_Distance_3D(float fx, float fy, float fz) {
  // this function computes the distance from the origin to x,y,z

  int32_t temp;    // used for swaping
  int32_t x, y, z;   // used for algorithm

  // make sure values are all positive
  x = fabs(fx) * 1024;
  y = fabs(fy) * 1024;
  z = fabs(fz) * 1024;

  // sort values
  if (y < x)
    SWAP3D(x, y, temp)

    if (z < y)
      SWAP3D(y, z, temp)

      if (y < x)
        SWAP3D(x, y, temp)

        int32_t dist = (z + 11 * (y >> 5) + (x >> 2));

  // compute distance with 8% error
  return ((float) (dist >> 10));

} //

////////////////////////////////////////////////////////////////

// these are the 4D version of the vector functions, they
// assume that the vectors are 3D with a w, so w is left
// out of all the operations

void VECTOR4D_Build(VECTOR4D_PTR init, VECTOR4D_PTR term, VECTOR4D_PTR result) {
  // build a 4d vector
  result->x = term->x - init->x;
  result->y = term->y - init->y;
  result->z = term->z - init->z;
  result->w = 1;

} // end VECTOR4D_Build

////////////////////////////////////////////////////////////////

void VECTOR4D_Add(VECTOR4D_PTR va, VECTOR4D_PTR vb, VECTOR4D_PTR vsum) {
  // this function adds va+vb and return it in vsum
  vsum->x = va->x + vb->x;
  vsum->y = va->y + vb->y;
  vsum->z = va->z + vb->z;
  vsum->w = 1;

} // end VECTOR4D_Add

////////////////////////////////////////////////////////////

VECTOR4D VECTOR4D_Add1(VECTOR4D_PTR va, VECTOR4D_PTR vb) {
  // this function adds va+vb and returns the result on
  // the stack
  VECTOR4D vsum;

  vsum.x = va->x + vb->x;
  vsum.y = va->y + vb->y;
  vsum.z = va->z + vb->z;
  vsum.w = 1;

  // return result
  return (vsum);

} // end VECTOR4D_Add

////////////////////////////////////////////////////////////

void VECTOR4D_Sub(VECTOR4D_PTR va, VECTOR4D_PTR vb, VECTOR4D_PTR vdiff) {
  // this function subtracts va-vb and return it in vdiff
  // the stack
  vdiff->x = va->x - vb->x;
  vdiff->y = va->y - vb->y;
  vdiff->z = va->z - vb->z;
  vdiff->w = 1;

} // end VECTOR4D_Sub

////////////////////////////////////////////////////////////

VECTOR4D VECTOR4D_Sub1(VECTOR4D_PTR va, VECTOR4D_PTR vb) {
  // this function subtracts va-vb and returns the result on
  // the stack
  VECTOR4D vdiff;

  vdiff.x = va->x - vb->x;
  vdiff.y = va->y - vb->y;
  vdiff.z = va->z - vb->z;
  vdiff.w = 1;

  // return result
  return (vdiff);

} // end VECTOR4D_Sub

////////////////////////////////////////////////////////////

void VECTOR4D_Scale(float k, VECTOR4D_PTR va) {
  // this function scales a vector by the constant k,
  // in place , note w is left unchanged

  // multiply each component by scaling factor
  va->x *= k;
  va->y *= k;
  va->z *= k;
  va->w = 1;

} // end VECTOR4D_Scale

/////////////////////////////////////////////////////////////

void VECTOR4D_Scale1(float k, VECTOR4D_PTR va, VECTOR4D_PTR vscaled) {
  // this function scales a vector by the constant k,
  // leaves the original unchanged, and returns the result
  // in vres as well as on the stack

  // multiply each component by scaling factor
  vscaled->x = k * va->x;
  vscaled->y = k * va->y;
  vscaled->z = k * va->z;
  vscaled->w = 1;

} // end VECTOR4D_Scale

//////////////////////////////////////////////////////////////

float VECTOR4D_Dot(VECTOR4D_PTR va, VECTOR4D_PTR vb) {
  // computes the dot product between va and vb
  return ((va->x * vb->x) + (va->y * vb->y) + (va->z * vb->z));
} // end VECTOR4D_DOT

/////////////////////////////////////////////////////////////

void VECTOR4D_Cross(VECTOR4D_PTR va, VECTOR4D_PTR vb, VECTOR4D_PTR vn) {
  // this function computes the cross product between va and vb
  // and returns the vector that is perpendicular to each in vn

  vn->x = ((va->y * vb->z) - (va->z * vb->y));
  vn->y = -((va->x * vb->z) - (va->z * vb->x));
  vn->z = ((va->x * vb->y) - (va->y * vb->x));
  vn->w = 1;

} // end VECTOR4D_Cross

/////////////////////////////////////////////////////////////

VECTOR4D VECTOR4D_Cross1(VECTOR4D_PTR va, VECTOR4D_PTR vb) {
  // this function computes the cross product between va and vb
  // and returns the vector that is perpendicular to each

  VECTOR4D vn;

  vn.x = ((va->y * vb->z) - (va->z * vb->y));
  vn.y = -((va->x * vb->z) - (va->z * vb->x));
  vn.z = ((va->x * vb->y) - (va->y * vb->x));
  vn.w = 1;

  // return result
  return (vn);

} // end VECTOR4D_Cross

//////////////////////////////////////////////////////////////

float VECTOR4D_Length(VECTOR4D_PTR va) {
  // computes the magnitude of a vector, slow

  return (sqrtf(va->x * va->x + va->y * va->y + va->z * va->z));

} // end VECTOR4D_Length

///////////////////////////////////////////////////////////////

float VECTOR4D_Length_Fast(VECTOR4D_PTR va) {
  // computes the magnitude of a vector using an approximation
  // very fast
  return (Fast_Distance_3D(va->x, va->y, va->z));

} // end VECTOR4D_Length_Fast

///////////////////////////////////////////////////////////////

void VECTOR4D_Normalize(VECTOR4D_PTR va) {
  // normalizes the sent vector and returns the result

  // compute length
  float length = sqrtf(va->x * va->x + va->y * va->y + va->z * va->z);

  // test for zero length vector
  // if found return zero vector
  if (length < EPSILON_E5)
    return;

  float length_inv = 1.0 / length;

  // compute normalized version of vector
  va->x *= length_inv;
  va->y *= length_inv;
  va->z *= length_inv;
  va->w = 1;

} // end VECTOR4D_Normalize

///////////////////////////////////////////////////////////////

void VECTOR4D_Normalize1(VECTOR4D_PTR va, VECTOR4D_PTR vn) {
  // normalizes the sent vector and returns the result in vn

  VECTOR4D_ZERO(vn);

  // compute length
  float length = sqrt(va->x * va->x + va->y * va->y + va->z * va->z);

  // test for zero length vector
  // if found return zero vector
  if (length < EPSILON_E5)
    return;

  float length_inv = 1.0 / length;

  // compute normalized version of vector
  vn->x = va->x * length_inv;
  vn->y = va->y * length_inv;
  vn->z = va->z * length_inv;
  vn->w = 1;

} // end VECTOR4D_Normalize

///////////////////////////////////////////////////////////////

float VECTOR4D_CosTh(VECTOR4D_PTR va, VECTOR4D_PTR vb) {
  // this function returns the cosine of the angle between
  // two vectors. Note, we could compute the actual angle,
  // many many times, in further calcs we will want ultimately
  // compute cos of the angle, so why not just leave it!
  return (VECTOR4D_Dot(va, vb) / (VECTOR4D_Length(va) * VECTOR4D_Length(vb)));

} // end VECTOR4D_CosTh

///////////////////////////////////////////////////////////////
int Mat_Mul1X2_3X2(MATRIX1X2_PTR ma, MATRIX3X2_PTR mb, MATRIX1X2_PTR mprod) {
  // this function multiplies a 1x2 matrix against a
  // 3x2 matrix - ma*mb and stores the result
  // using a dummy element for the 3rd element of the 1x2
  // to make the matrix multiply valid i.e. 1x3 X 3x2
  int col = 0;
  int index = 0;
  for (col = 0; col < 2; col++) {
    // compute dot product from row of ma
    // and column of mb

    float sum = 0;     // used to hold result

    for (index = 0; index < 2; index++) {
      // add in next product pair
      sum += (ma->M[index] * mb->M[index][col]);
    }     // end for index

    // add in last element * 1
    sum += mb->M[index][col];

    // insert resulting col element
    mprod->M[col] = sum;

  }   // end for col

  return (1);

} // end Mat_Mul_1X2_3X2

///////////////////////////////////////////////////////////////

void Mat_Add_4X4(MATRIX4X4_PTR ma, MATRIX4X4_PTR mb, MATRIX4X4_PTR msum) {
  // this function adds two 4x4 matrices together and
  // and stores the result

  for (int32_t row = 0; row < 4; row++) {
    for (int32_t col = 0; col < 4; col++) {
      // insert resulting row,col element
      msum->M[row][col] = ma->M[row][col] + mb->M[row][col];
    }     // end for col

  }   // end for row

} // end Mat_Add_4X4

///////////////////////////////////////////////////////////////

void Mat_Mul_4X4(MATRIX4X4_PTR ma, MATRIX4X4_PTR mb, MATRIX4X4_PTR mprod) {
  // this function multiplies two 4x4 matrices together and
  // and stores the result in mprod
  // note later we will take advantage of the fact that we know
  // that w=1 always, and that the last column of a 4x4 is
  // always 0

  for (int32_t row = 0; row < 4; row++) {
    for (int32_t col = 0; col < 4; col++) {
      // compute dot product from row of ma
      // and column of mb

      float sum = 0;       // used to hold result

      for (int32_t index = 0; index < 4; index++) {
        // add in next product pair
        sum += (ma->M[row][index] * mb->M[index][col]);
      }       // end for index

      // insert resulting row,col element
      mprod->M[row][col] = sum;

    }     // end for col

  }   // end for row

} // end Mat_Mul_4X4

////////////////////////////////////////////////////////////////

void Mat_Mul_1X4_4X4(MATRIX1X4_PTR ma, MATRIX4X4_PTR mb, MATRIX1X4_PTR mprod) {
  // this function multiplies a 1x4 matrix against a
  // 4x4 matrix - ma*mb and stores the result
  // no tricks or assumptions here, just a straight multiply

  for (int32_t col = 0; col < 4; col++) {
    // compute dot product from row of ma
    // and column of mb
    float sum = 0;     // used to hold result

    for (int32_t row = 0; row < 4; row++) {
      // add in next product pair
      sum += (ma->M[row] * mb->M[row][col]);
    }     // end for index

    // insert resulting col element
    mprod->M[col] = sum;

  }   // end for col

} // end Mat_Mul_1X4_4X4

////////////////////////////////////////////////////////////////////

void Mat_Mul_VECTOR3D_4X4(VECTOR3D_PTR va, MATRIX4X4_PTR mb, VECTOR3D_PTR vprod) {
  // this function multiplies a VECTOR3D against a
  // 4x4 matrix - ma*mb and stores the result in mprod
  // the function assumes that the vector refers to a
  // 4D homogenous vector, thus the function assumes that
  // w=1 to carry out the multiply, also the function
  // does not carry out the last column multiply since
  // we are assuming w=1, there is no point
  int32_t col, row;
  for (col = 0; col < 3; col++) {
    // compute dot product from row of ma
    // and column of mb
    float sum = 0;     // used to hold result

    for (row = 0; row < 3; row++) {
      // add in next product pair
      sum += (va->M[row] * mb->M[row][col]);
    }     // end for index

    // add in last element in column or w*mb[3][col]
    sum += mb->M[row][col];

    // insert resulting col element
    vprod->M[col] = sum;

  }   // end for col

} // end Mat_Mul_VECTOR3D_4X4

///////////////////////////////////////////////////////////////

void Mat_Mul_VECTOR3D_4X3(VECTOR3D_PTR va, MATRIX4X3_PTR mb, VECTOR3D_PTR vprod) {
  // this function multiplies a VECTOR3D against a
  // 4x3 matrix - ma*mb and stores the result in mprod
  // the function assumes that the vector refers to a
  // 4D homogenous vector, thus the function assumes that
  // w=1 to carry out the multiply, also the function
  // does not carry out the last column multiply since
  // we are assuming w=1, there is no point
  int32_t col, row;
  for (col = 0; col < 3; col++) {
    // compute dot product from row of ma
    // and column of mb
    float sum = 0;     // used to hold result

    for (row = 0; row < 3; row++) {
      // add in next product pair
      sum += (va->M[row] * mb->M[row][col]);
    }     // end for index

    // add in last element in column or w*mb[3][col]
    sum += mb->M[row][col];

    // insert resulting col element
    vprod->M[col] = sum;

  }   // end for col

} // end Mat_Mul_VECTOR3D_4X3

////////////////////////////////////////////////////////////////////

void Mat_Mul_VECTOR4D_4X4(VECTOR4D_PTR va, MATRIX4X4_PTR mb, VECTOR4D_PTR vprod) {
// this function multiplies a VECTOR4D against a
// 4x4 matrix - ma*mb and stores the result in mprod
// the function makes no assumptions

  for (int32_t col = 0; col < 4; col++) {
    // compute dot product from row of ma
    // and column of mb
    float sum = 0;     // used to hold result

    for (int32_t row = 0; row < 4; row++) {
      // add in next product pair
      sum += (va->M[row] * mb->M[row][col]);
    }     // end for index

    // insert resulting col element
    vprod->M[col] = sum;

  }   // end for col

} // end Mat_Mul_VECTOR4D_4X4

////////////////////////////////////////////////////////////////////

void Mat_Mul_VECTOR4D_4X3(VECTOR4D_PTR va, MATRIX4X4_PTR mb, VECTOR4D_PTR vprod) {
// this function multiplies a VECTOR4D against a
// 4x3 matrix - ma*mb and stores the result in mprod
// the function assumes that the last column of
// mb is [0 0 0 1]t , thus w just gets replicated
// from the vector [x y z w]

  for (int32_t col = 0; col < 3; col++) {
    // compute dot product from row of ma
    // and column of mb
    float sum = 0;     // used to hold result

    for (int32_t row = 0; row < 4; row++) {
      // add in next product pair
      sum += (va->M[row] * mb->M[row][col]);
    }     // end for index

    // insert resulting col element
    vprod->M[col] = sum;

  }   // end for col

  // copy back w element
  vprod->M[3] = va->M[3];

} // end Mat_Mul_VECTOR4D_4X3

///////////////////////////////////////////////////////////////

void Mat_Init_4X4(MATRIX4X4_PTR ma, float m00, float m01, float m02, float m03,
                  float m10, float m11, float m12, float m13, float m20, float m21,
                  float m22, float m23, float m30, float m31, float m32, float m33) {
  // this function fills a 4x4 matrix with the sent data in
  // row major form
  ma->M00 = m00;
  ma->M01 = m01;
  ma->M02 = m02;
  ma->M03 = m03;
  ma->M10 = m10;
  ma->M11 = m11;
  ma->M12 = m12;
  ma->M13 = m13;
  ma->M20 = m20;
  ma->M21 = m21;
  ma->M22 = m22;
  ma->M23 = m23;
  ma->M30 = m30;
  ma->M31 = m31;
  ma->M32 = m32;
  ma->M33 = m33;

} // end Mat_Init_4X4

////////////////////////////////////////////////////////////////

int32_t Mat_Inverse_4X4(MATRIX4X4_PTR m, MATRIX4X4_PTR mi) {
  // computes the inverse of a 4x4, assumes that the last
  // column is [0 0 0 1]t

  float det = (m->M00 * (m->M11 * m->M22 - m->M12 * m->M21)
               - m->M01 * (m->M10 * m->M22 - m->M12 * m->M20)
               + m->M02 * (m->M10 * m->M21 - m->M11 * m->M20));

  // test determinate to see if it's 0
  if (fabs(det) < EPSILON_E5)
    return (0);

  float det_inv = 1.0f / det;

  mi->M00 = det_inv * (m->M11 * m->M22 - m->M12 * m->M21);
  mi->M01 = -det_inv * (m->M01 * m->M22 - m->M02 * m->M21);
  mi->M02 = det_inv * (m->M01 * m->M12 - m->M02 * m->M11);
  mi->M03 = 0.0f;   // always 0

  mi->M10 = -det_inv * (m->M10 * m->M22 - m->M12 * m->M20);
  mi->M11 = det_inv * (m->M00 * m->M22 - m->M02 * m->M20);
  mi->M12 = -det_inv * (m->M00 * m->M12 - m->M02 * m->M10);
  mi->M13 = 0.0f;   // always 0

  mi->M20 = det_inv * (m->M10 * m->M21 - m->M11 * m->M20);
  mi->M21 = -det_inv * (m->M00 * m->M21 - m->M01 * m->M20);
  mi->M22 = det_inv * (m->M00 * m->M11 - m->M01 * m->M10);
  mi->M23 = 0.0f;   // always 0

  mi->M30 = -(m->M30 * mi->M00 + m->M31 * mi->M10 + m->M32 * mi->M20);
  mi->M31 = -(m->M30 * mi->M01 + m->M31 * mi->M11 + m->M32 * mi->M21);
  mi->M32 = -(m->M30 * mi->M02 + m->M31 * mi->M12 + m->M32 * mi->M22);
  mi->M33 = 1.0f;   // always 0

  // return success
  return (1);

} // end Mat_Inverse_4X4

///////////////////////////////////////////////////////////////

void PLANE3D_Init(PLANE3D_PTR plane, POINT3D_PTR p0, VECTOR3D_PTR normal,
                  int32_t normalize) {
// this function initializes a 3d plane

// copy the point
  POINT3D_COPY(&plane->p0, p0);

// if normalize is 1 then the normal is made into a unit vector
  if (!normalize)
    VECTOR3D_COPY(&plane->n, normal);
  else {
    // make normal into unit vector
    VECTOR3D_Normalize(normal, &plane->n);
  }   // end else

} // end PLANE3D_Init

void Build_Sin_Cos_Tables(void) {

// create sin/cos lookup table
// note the creation of one extra element; 360
// this helps with logic in using the tables

// generate the tables 0 - 360 inclusive
  for (int ang = 0; ang <= 360; ang++) {
    // convert ang to radians
    float theta = ((float) ang) * XPI / 180.0;
    // insert next entry into table
    my_cos_look[ang] = cosf(theta);
    my_sin_look[ang] = sinf(theta);

  }   // end for ang

} // end Build_Sin_Cos_Tables

float Fast_Sin(float theta) {
// this function uses the sin_look[] lookup table, but
// has logic to handle negative angles as well as fractional
// angles via interpolation, use this for a more robust
// sin computation that the blind lookup, but with with
// a slight hit in speed

// convert angle to 0-359
  theta = fmodf(theta, 360);

// make angle positive
  if (theta < 0)
    theta += 360.0;

// compute floor of theta and fractional part to interpolate
  int theta_int = (int) theta;
  float theta_frac = theta - theta_int;

// now compute the value of sin(angle) using the lookup tables
// and interpolating the fractional part, note that if theta_int
// is equal to 359 then theta_int+1=360, but this is fine since the
// table was made with the entries 0-360 inclusive
  return (my_sin_look[theta_int]
          + theta_frac * (my_sin_look[theta_int + 1] - my_sin_look[theta_int]));

} // end Fast_Sin

///////////////////////////////////////////////////////////////

float Fast_Cos(float theta) {
// this function uses the cos_look[] lookup table, but
// has logic to handle negative angles as well as fractional
// angles via interpolation, use this for a more robust
// cos computation that the blind lookup, but with with
// a slight hit in speed

// convert angle to 0-359
  theta = fmodf(theta, 360);

// make angle positive
  if (theta < 0)
    theta += 360.0;

// compute floor of theta and fractional part to interpolate
  int theta_int = (int) theta;
  float theta_frac = theta - theta_int;

// now compute the value of sin(angle) using the lookup tables
// and interpolating the fractional part, note that if theta_int
// is equal to 359 then theta_int+1=360, but this is fine since the
// table was made with the entries 0-360 inclusive
  return (my_cos_look[theta_int]
          + theta_frac * (my_cos_look[theta_int + 1] - my_cos_look[theta_int]));

} // end Fast_Cos

