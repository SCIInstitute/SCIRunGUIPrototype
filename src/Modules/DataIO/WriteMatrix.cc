/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2012 Scientific Computing and Imaging Institute,
   University of Utah.

   License for the specific language governing rights and limitations under
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
/// @todo Documentation Modules/DataIO/WriteMatrix.cc

#include <Modules/DataIO/WriteMatrix.h>
#include <Core/Algorithms/Base/AlgorithmVariableNames.h>
#include <Core/Datatypes/String.h>
#include <Core/Datatypes/DenseMatrix.h>

using namespace SCIRun::Modules::DataIO;
using namespace SCIRun::Core::Algorithms;
using namespace SCIRun::Core::Datatypes;
using namespace SCIRun::Dataflow::Networks;
using namespace SCIRun::Modules::DataIO;

WriteMatrixModule::WriteMatrixModule() : Module(ModuleLookupInfo("WriteMatrix", "DataIO", "SCIRun"))
{
  INITIALIZE_PORT(Filename);
  INITIALIZE_PORT(MatrixToWrite);
}

/// @todo: unit test. Requires algorithm injection/factory for mocking, to be able to isolate the "optional file argument" part.
void WriteMatrixModule::execute()
{
  auto matrix = getRequiredInput(MatrixToWrite);

  auto fileOption = getOptionalInput(Filename);

  if (fileOption && *fileOption)
  {
    get_state()->setValue(SCIRun::Core::Algorithms::Variables::Filename, (*fileOption)->value());
  }
  auto path = get_state()->getValue(Variables::Filename).getFilename();
  filename_ = path.string();

  if (needToExecute())
  {
    algo().set(Variables::Filename, filename_);
    algo().run_generic(make_input((MatrixToWrite, matrix)));
  }
}
