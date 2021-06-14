/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2020 Scientific Computing and Imaging Institute,
   University of Utah.

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/


#include <gtest/gtest.h>
#include <Core/Algorithms/Base/Option.h>
#include <Core/Datatypes/DatatypeFwd.h>
#include <Core/Datatypes/Legacy/Field/CastFData.h>
#include <Core/Datatypes/Legacy/Field/FieldInformation.h>
#include <Core/Datatypes/Legacy/Field/Mesh.h>
#include <Core/Datatypes/Legacy/Field/VField.h>
#include <Core/GeometryPrimitives/Tensor.h>
#include <Core/GeometryPrimitives/Point.h>
#include <Core/GeometryPrimitives/Vector.h>
#include <Core/Datatypes/Dyadic3DTensor.h>
#include <Core/Algorithms/Base/AlgorithmData.h>
#include <Core/Algorithms/Math/ComputeTensorUncertaintyAlgorithm.h>
#include <boost/smart_ptr/make_shared_object.hpp>
#include "Core/Datatypes/MatrixFwd.h"

using namespace SCIRun;
using namespace SCIRun::Core::Datatypes;
using namespace SCIRun::Core::Geometry;
using namespace SCIRun::Core::Algorithms;
using namespace SCIRun::Core::Algorithms::Math;

const auto DIM = 3;
const std::set<std::string> options = {"Linear Invariant","Log-Euclidean","Matrix Average"};
const auto MATRIX_AVERAGE = AlgoOption("Matrix Average", options);
const auto LOG_EUCLIDEAN = AlgoOption("Log-Euclidean", options);
const auto LINEAR_INVARIANT = AlgoOption("Linear Invariant", options);

FieldHandle tensorToField(Tensor t)
{
  FieldInformation lfi("LatVolMesh", 1, "Tensor");
  MeshHandle mesh = CreateMesh(lfi);
  FieldHandle fh = CreateField(lfi, mesh);
  fh->vfield()->set_all_values(t);
  return fh;
}

void assertTensorsNear(const Tensor& t, const Tensor& expected, double epsilon)
{
  for (int i = 0; i < DIM; ++i)
    for (int j = 0; j < DIM; ++j)
      ASSERT_NEAR(t.val(i, j), expected.val(i, j), epsilon);
}

TEST(ComputeTensorUncertaintyTest, MatrixAverage)
{
  const double unitHalf = 0.5 * std::sqrt(2);
  auto fh1 = tensorToField(Tensor(5*Vector(1,0,0), 3*Vector(0,1,0), Vector(0,0,1)));
  auto fh2 = tensorToField(Tensor(7*Vector(unitHalf,unitHalf,0), 2*Vector(unitHalf,-unitHalf,0), Vector(0,0,1)));
  Tensor t_expected(4.75, 1.25, 0, 3.75, 0, 1);
  const auto epsilon = 1.0e-12;

  FieldList fields = {fh1, fh2};
  ComputeTensorUncertaintyAlgorithm algo;
  boost::tuple<FieldHandle, MatrixHandle> output = algo.runImpl(fields, MATRIX_AVERAGE, MATRIX_AVERAGE);

  Tensor t;
  output.get<0>()->vfield()->get_value(t,0);
  assertTensorsNear(t, t_expected, epsilon);
}

// I hand calculated the Log-Euclidean calcuation except the last step of matrix exponential, hence the large epsilon
TEST(ComputeTensorUncertaintyTest, LogEuclidean)
{
  const double unitHalf = 0.5 * std::sqrt(2);
  auto fh1 = tensorToField(Tensor(5*Vector(1,0,0), 3*Vector(0,1,0), Vector(0,0,1)));
  auto fh2 = tensorToField(Tensor(7*Vector(unitHalf,unitHalf,0), 2*Vector(unitHalf,-unitHalf,0), Vector(0,0,1)));
  Tensor t_expected(4.522, 1.215, 0, 3.531, 0, 1);
  const auto epsilon = 0.001;

  FieldList fields = {fh1, fh2};
  ComputeTensorUncertaintyAlgorithm algo;
  boost::tuple<FieldHandle, MatrixHandle> output = algo.runImpl(fields, LOG_EUCLIDEAN, LOG_EUCLIDEAN);

  Tensor t;
  output.get<0>()->vfield()->get_value(t,0);
  assertTensorsNear(t, t_expected, epsilon);
}

// Since this only tests the invariant calculation, we only need to check the eigenvalues
TEST(ComputeTensorUncertaintyTest, LinearInvariant)
{
  const double unitHalf = 0.5 * std::sqrt(2);
  auto fh1 = tensorToField(Tensor(5*Vector(1,0,0), 3*Vector(0,1,0), Vector(0,0,1)));
  auto fh2 = tensorToField(Tensor(7*Vector(unitHalf,unitHalf,0), 2*Vector(unitHalf,-unitHalf,0), Vector(0,0,1)));
  std::vector<double> eigs_expected = {5.9075, 2.715, 0.877};
  const auto epsilon = 0.003;

  FieldList fields = {fh1, fh2};
  ComputeTensorUncertaintyAlgorithm algo;
  boost::tuple<FieldHandle, MatrixHandle> output = algo.runImpl(fields, LINEAR_INVARIANT, MATRIX_AVERAGE);

  Tensor t;
  output.get<0>()->vfield()->get_value(t,0);
  std::vector<double> eigs(3);
  t.get_eigenvalues(eigs[0], eigs[1], eigs[2]);

  for (auto i = 0; i < DIM; ++i)
    ASSERT_NEAR(eigs[i], eigs_expected[i], epsilon);
}
