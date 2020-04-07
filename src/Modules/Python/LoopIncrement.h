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


#ifndef MODULES_PYTHON_LoopIncrement_H
#define MODULES_PYTHON_LoopIncrement_H

#include <Dataflow/Network/Module.h>
#include <Core/Thread/Mutex.h>
#include <Modules/Python/share.h>

namespace SCIRun
{
  namespace Core
  {
    namespace Algorithms
    {
      namespace Python
      {

      }
    }
  }

  namespace Modules
  {
    namespace Python
    {
      class SCISHARE LoopIncrement : public SCIRun::Dataflow::Networks::Module,
        public Has1InputPort<MetadataObjectPortTag>,
        public Has1OutputPort<MetadataObjectPortTag>
      {
      public:
        LoopIncrement();
        void execute() override;
        void setStateDefaults() override;
        void postStateChangeInternalSignalHookup() override;
        // HAS_DYNAMIC_PORTS
        // INPUT_PORT_DYNAMIC(0, InputMatrix, Matrix);
        // INPUT_PORT_DYNAMIC(1, InputField, Field);
        INPUT_PORT(0, LoopEndCodeObject, MetadataObject);
        OUTPUT_PORT(0, LoopIncrementCodeObject, MetadataObject);
        // OUTPUT_PORT(1, PythonMatrix2, Matrix);
        // OUTPUT_PORT(2, PythonMatrix3, Matrix);
        // OUTPUT_PORT(3, PythonField1, Field);
        // OUTPUT_PORT(4, PythonField2, Field);
        // OUTPUT_PORT(5, PythonField3, Field);
        // OUTPUT_PORT(6, PythonString1, String);
        // OUTPUT_PORT(7, PythonString2, String);
        // OUTPUT_PORT(8, PythonString3, String);

        MODULE_TRAITS_AND_INFO(ModuleHasUI)
        NEW_HELP_WEBPAGE_ONLY
        #ifndef BUILD_WITH_PYTHON
          DISABLED_WITHOUT_ABOVE_COMPILE_FLAG
        #endif
      private:
        //SharedPointer<Core::Algorithms::Python::InterfaceWithPythonCodeTranslator> translator_;
      };

    }
  }
}

#endif
