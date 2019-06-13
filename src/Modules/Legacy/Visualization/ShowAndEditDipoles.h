/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2015 Scientific Computing and Imaging Institute,
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

#ifndef MODULES_LEGACY_VISUALIZATION_SHOWANDEDITDIPOLES_H
#define MODULES_LEGACY_VISUALIZATION_SHOWANDEDITDIPOLES_H

#include <Dataflow/Network/GeometryGeneratingModule.h>
#include <Core/Datatypes/Geometry.h>
#include <Graphics/Widgets/Widget.h>
#include <Modules/Legacy/Visualization/share.h>

namespace SCIRun {
  namespace Modules {
    namespace Visualization {

      class SCISHARE ShowAndEditDipoles : public SCIRun::Dataflow::Networks::GeometryGeneratingModule,
        public Has1InputPort<FieldPortTag>,
        public Has2OutputPorts<FieldPortTag, GeometryPortTag>
      {
      public:
        ShowAndEditDipoles();
        virtual void execute() override;
        virtual void setStateDefaults() override;

        static const Core::Algorithms::AlgorithmParameterName WidgetSize;
        static const Core::Algorithms::AlgorithmParameterName Sizing;
        static const Core::Algorithms::AlgorithmParameterName ShowLastAsVector;
        static const Core::Algorithms::AlgorithmParameterName ShowLines;
        static const Core::Algorithms::AlgorithmParameterName PointPositions;

        INPUT_PORT(0, DipoleInputField, Field);
        OUTPUT_PORT(0, DipoleOutputField, Field);
        OUTPUT_PORT(1, DipoleWidget, GeometryObject);

        MODULE_TRAITS_AND_INFO(ModuleHasUI);

      private:
        boost::shared_ptr<class ShowAndEditDipolesImpl> impl_;
        Core::Geometry::Point pos_;
        Core::Geometry::Vector direction_;
        double sphereRadius_;
        double cylinderRadius_;
        double coneRadius_;
        double diskRadius_;
        double diskDistFromCenter_;
        double diskWidth_;
        int widgetID_;

        Core::Datatypes::ColorRGB deflPointCol_;
        Core::Datatypes::ColorRGB deflCol_;
        Core::Datatypes::ColorRGB greenCol_;
        Core::Datatypes::ColorRGB resizeCol_;
        // Core::Geometry::Point currentLocation() const;
        // Graphics::Datatypes::GeometryHandle buildGeometryObject(FieldHandle field, const GeometryIDGenerator& idGenerator);
        FieldHandle GenerateOutputField();
        void processWidgetFeedback(const Core::Datatypes::ModuleFeedback& var);
        void adjustPositionFromTransform(const Core::Geometry::Transform& transformMatrix, int index);
        int moveCount_ {0};
        // void setNearestNode(const Core::Geometry::Point& location);
        // void setNearestElement(const Core::Geometry::Point& location);
      };
    }
  }
};

#endif
