/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2009 Scientific Computing and Imaging Institute,
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

/*
 *  ShowAndEditDipoles.cc:  Builds the RHS of the FE matrix for current sources
 *
 *  Written by:
 *   David Weinstein
 *   University of Utah
 *   May 1999
 *
 */

#include <Core/Algorithms/Visualization/RenderFieldState.h>
#include <Core/Datatypes/Color.h>
#include <Core/Datatypes/DenseMatrix.h>
#include <Core/Datatypes/Geometry.h>
#include <Core/Datatypes/Legacy/Field/Field.h>
#include <Core/Datatypes/Legacy/Field/FieldInformation.h>
#include <Core/Datatypes/Legacy/Field/Mesh.h>
#include <Core/Datatypes/Legacy/Field/VField.h>
#include <Core/Datatypes/Mesh/MeshFacade.h>
#include <Core/GeometryPrimitives/Point.h>
#include <Core/Logging/Log.h>
#include <Graphics/Glyphs/GlyphGeom.h>
#include <Graphics/Widgets/ArrowWidget.h>
#include <Modules/Legacy/Visualization/ShowAndEditDipoles.h>
#include <math.h>

using namespace SCIRun;
using namespace Core;
using namespace Logging;
using namespace Datatypes;
using namespace Algorithms;
using namespace Geometry;
using namespace Graphics;
using namespace Modules::Visualization;
using namespace Dataflow::Networks;
using namespace Graphics::Datatypes;

MODULE_INFO_DEF(ShowAndEditDipoles, Visualization, SCIRun)

ShowAndEditDipoles::ShowAndEditDipoles()
  : GeometryGeneratingModule(staticInfo_)
{
  INITIALIZE_PORT(DipoleInputField);
  INITIALIZE_PORT(DipoleOutputField);
  INITIALIZE_PORT(DipoleWidget);

  firstRun_ = true;
  getFromFile_ = false;
  lastVectorShown_ = false;
  previousSizing_ = SizingType::ORIGINAL;

  deflPointCol_ = ColorRGB(0.54, 0.6, 1.0);
  deflCol_ = ColorRGB(0.5, 0.5, 0.5);
  greenCol_ = ColorRGB(0.2, 0.8, 0.2);
  resizeCol_ = ColorRGB(0.54, 1.0, 0.60);
  lineCol_ = ColorRGB(0.8, 0.8, 0.2);

  widgetIter_ = 0;
  resolution_ = 20;
  previousScaleFactor_ = 0.0;
  zeroVectorRescale_ = 1.0e-3;
}

void ShowAndEditDipoles::setStateDefaults()
{
  auto state = get_state();
  state->setValue(FieldName, std::string());
  state->setValue(WidgetScaleFactor, 1.0);
  state->setValue(Sizing, SizingType::ORIGINAL);
  state->setValue(ShowLastAsVector, false);
  state->setValue(MoveDipolesTogether, false);
  state->setValue(ShowLines, false);
  state->setValue(Reset, false);
  state->setValue(DataSaved, false);
  state->setValue(DipolePositions, VariableList());
  state->setValue(DipoleDirections, VariableList());
  state->setValue(DipoleScales, VariableList());
  state->setValue(LargestSize, 0.0);

  getOutputPort(DipoleWidget)->connectConnectionFeedbackListener([this](const ModuleFeedback& var) { processWidgetFeedback(var); });
}

void ShowAndEditDipoles::execute()
{
  FieldHandle fh = getRequiredInput(DipoleInputField);
  FieldInformation fi(fh);
  auto state = get_state();

  // Point clouds must be linear, so we only loook for node-based data
  if (!(fi.is_pointcloudmesh()))
  {
    error("Input field was not a valid point cloud.");
    return;
  }
  else if(!fi.is_vector())
  {
    error("Input field does not contain vectors.");
    return;
  }

  loadData();

  // Recreate all dipoles only if all were altered
  if(inputsChanged()
     || state->getValue(Reset).toBool()
     || previousSizing_ != state->getValue(Sizing).toInt()
     || previousScaleFactor_ != state->getValue(WidgetScaleFactor).toDouble()
     || state->getValue(MoveDipolesTogether).toBool()
     || getFromFile_)
  {
    refreshGeometry();
  }

  // Only run if Show Last as Vector is toggled
  if(lastVectorShown_ != state->getValue(ShowLastAsVector).toBool())
  {
    toggleLastVectorShown();
  }

  generateGeomsList();

  sendOutput(DipoleOutputField, makePointCloud());

  std::string idName = "SAEDField"
    + GeometryObject::delimiter +
    state->getValue(ShowAndEditDipoles::FieldName).toString() +
    " (from " + id().id_ +")" +
    "(" + std::to_string(widgetIter_) + ")";

  // Generate composite geometry object
  auto comp_geo = createGeomComposite(*this, "dipoles", geoms_.begin(), geoms_.end());
  sendOutput(DipoleWidget, comp_geo);
  widgetIter_++;

  saveToParameters();
}

void ShowAndEditDipoles::loadData()
{
  auto state = get_state();

  // Get new data upstream or load from existing data
  getFromFile_ = firstRun_ && state->getValue(DataSaved).toBool();

  if(getFromFile_
     && !state->getValue(Reset).toBool())
  {
    loadFromParameters();
    makeScalesPositive();
    firstRun_ = false;
  }
  else
  {
    if(inputsChanged()
       || state->getValue(Reset).toBool())
    {
      ReceiveInputField();
      makeScalesPositive();
    }

    // Scaling
    if(previousSizing_ != state->getValue(Sizing).toInt()
       || previousScaleFactor_ != state->getValue(WidgetScaleFactor).toDouble())
    {
      ReceiveInputScales();
      makeScalesPositive();
    }
  }

  bool zeroVector = false;
  for(size_t i = 0; i < scale_.size(); i++)
  {
    if(scale_[i] == 0.0)
    {
      scale_[i] = zeroVectorRescale_;
      direction_[i] = Vector(1, 1, 1).normal();
      zeroVector = true;
    }
  }

  if((previousSizing_ != state->getValue(Sizing).toInt() || state->getValue(Reset).toBool())
     && state->getValue(Sizing).toInt() == SizingType::NORMALIZE_VECTOR_DATA)
  {
    scale_.clear();
    for(size_t i = 0; i < pos_.size(); i++)
    {
      scale_[i] = 1.0;
    }
  }

  if(zeroVector)
  {
    warning("Input data contains zero vectors.");
  }
}



void ShowAndEditDipoles::loadFromParameters()
{
  auto state = get_state();
  VariableList positions = state->getValue(DipolePositions).toVector();
  VariableList directions = state->getValue(DipoleDirections).toVector();
  VariableList scales = state->getValue(DipoleScales).toVector();

  pos_.resize(positions.size());
  direction_.resize(positions.size());
  scale_.resize(positions.size());
  for(size_t i = 0; i < positions.size(); i++)
  {
    pos_[i] = pointFromString(positions[i].toString());
    direction_[i] = vectorFromString(directions[i].toString());
    scale_[i] = scales[i].toDouble();
  }
}

void ShowAndEditDipoles::saveToParameters()
{
  auto state = get_state();

  VariableList positions;
  VariableList directions;
  VariableList scales;
  for(size_t i = 0; i < pos_.size(); i++)
  {
    positions.push_back(makeVariable("dip_pos", pos_[i].get_string()));
    directions.push_back(makeVariable("dip_dir", direction_[i].get_string()));
    scales.push_back(makeVariable("dip_scale", scale_[i]));
  }
  state->setValue(DipolePositions, positions);
  state->setValue(DipoleDirections, directions);
  state->setValue(DipoleScales, scales);
  state->setValue(DataSaved, true);
}

void ShowAndEditDipoles::refreshGeometry()
{
  auto state = get_state();
  // Garbage collect
  for(auto dip : pointWidgets_)
  {
    dip->erase(dip->begin(), dip->end());
  }
  pointWidgets_.erase(pointWidgets_.begin(), pointWidgets_.end());

  GenerateOutputGeom();
  state->setValue(Reset, false);
  previousSizing_ = static_cast<SizingType>(state->getValue(Sizing).toInt());
  previousScaleFactor_ = state->getValue(WidgetScaleFactor).toDouble();
}

void ShowAndEditDipoles::toggleLastVectorShown()
{
  FieldHandle fh = getRequiredInput(DipoleInputField);
  auto bbox = fh->vmesh()->get_bounding_box();
  auto state = get_state();

  size_t last_id = pos_.size() - 1;
  // Destroy last dipole
  pointWidgets_[last_id]->erase(pointWidgets_[last_id]->begin(), pointWidgets_[last_id]->end());

  double scale = scale_[last_id];
  if(state->getValue(Sizing).toInt() == SizingType::NORMALIZE_BY_LARGEST_VECTOR)
    scale /= state->getValue(LargestSize).toDouble();

  // createDipoleWidget(bbox, pos_[last_id], direction_[last_id], scale * state->getValue(WidgetScaleFactor).toDouble(), last_id, state->getValue(ShowLastAsVector).toBool());
  ArrowWidget arrow(*this, "SAED", scale * state->getValue(WidgetScaleFactor).toDouble(),
                    pos_[last_id], direction_[last_id], resolution_,
                    lastVectorShown_, last_id, ++widgetIter_, bbox);
  lastVectorShown_ = state->getValue(ShowLastAsVector).toBool();
}

void ShowAndEditDipoles::generateGeomsList()
{
  auto state = get_state();
  // Recreate geom list
  geoms_.resize(0);

  // Rewrite all existing geom
  for(size_t d = 0; d < pointWidgets_.size(); d++)
  {
    for(size_t w = 0; w < pointWidgets_[d]->size(); w++)
      geoms_.push_back((*pointWidgets_[d])[w]);
  }
  if(state->getValue(ShowLines).toBool())
    geoms_.push_back(addLines());
}

void ShowAndEditDipoles::processWidgetFeedback(const ModuleFeedback& var)
{
  try
  {
    auto vsf = dynamic_cast<const ViewSceneFeedback&>(var);
    if (vsf.matchesWithModuleId(id()))
    {
      size_t widgetType;
      size_t widgetID;
      try
      {
      // Check if correct widget type
        static boost::regex r("ArrowWidget((.+)).+");
        boost::smatch what;
        regex_match(vsf.selectionName, what, r);

        // Get widget index and id
        static boost::regex ind_r("\\([0-9]*\\)");
        boost::smatch match;
        std::string::const_iterator searchStart(vsf.selectionName.cbegin());
        std::vector<std::string> matches;

        // Find all matches
        while(regex_search(searchStart, vsf.selectionName.cend(), match, ind_r))
        {
          matches.push_back(match[0]);
          searchStart = match.suffix().first;
        }

        // Remove parantheses
        for(size_t i = 0; i < matches.size(); i++)
        {
          matches[i] = matches[i].substr(1, matches[i].length()-2);
        }
        // Cast to size_t
        widgetType = boost::lexical_cast<size_t>(matches[0]);
        widgetID = boost::lexical_cast<size_t>(matches[1]);
        adjustPositionFromTransform(vsf.transform, widgetType, widgetID);
      }
      catch (...)
      {
        logWarning("Failure parsing widget id");
      }
      enqueueExecuteAgain(false);
    }
  }
  catch (std::bad_cast&)
  {
    //ignore
  }
}

void ShowAndEditDipoles::calculatePointMove(Point& oldPos, Point& newPos)
{
  auto state = get_state();

  if(state->getValue(MoveDipolesTogether).toBool())
  {
    Vector dist = newPos - oldPos;
    for(size_t i = 0; i < pos_.size(); i++)
    {
      pos_[i] += dist;
    }
  }
  else
    oldPos = newPos;
}

void ShowAndEditDipoles::adjustPositionFromTransform(const Transform& transformMatrix, size_t type, size_t id)
{
  auto state = get_state();
  DenseMatrix center(4, 1);

  auto currLoc = (*pointWidgets_[id])[type]->position();
  center << currLoc.x(), currLoc.y(), currLoc.z(), 1.0;
  DenseMatrix newTransform(DenseMatrix(transformMatrix) * center);

  Point transformPoint(newTransform(0, 0) / newTransform(3, 0),
                       newTransform(1, 0) / newTransform(3, 0),
                       newTransform(2, 0) / newTransform(3, 0));

  switch (type)
  {
  // Sphere and Cylinder reposition dipole
  case ArrowWidgetSection::SPHERE:
    calculatePointMove(pos_[id], transformPoint);
    break;
  case ArrowWidgetSection::CYLINDER:
  {
    double widgetScale = get_state()->getValue(WidgetScaleFactor).toDouble();
    // Shift direction back because newPos is the center of the cylinder
    Point newPos = transformPoint - 1.375 * direction_[id] * scale_[id] * widgetScale * sphereRadius_;
    calculatePointMove(pos_[id], newPos);
    break;
  }
  // Cone rotates dipole
  case ArrowWidgetSection::CONE:
    direction_[id] = (transformPoint - pos_[id]).normal();
    break;
  // Disk resizes dipole
  case ArrowWidgetSection::DISK:
  {
    Vector newVec(transformPoint - pos_[id]);
    newVec /= diskDistFromCenter_;

    double widgetScale = get_state()->getValue(WidgetScaleFactor).toDouble();
    scale_[id] = Dot(newVec, direction_[id]) / widgetScale;

    if(state->getValue(Sizing).toInt() == SizingType::NORMALIZE_BY_LARGEST_VECTOR)
      scale_[id] *= state->getValue(LargestSize).toDouble();

    makeScalesPositive();
    break;
  }
  }
  FieldHandle fh = getRequiredInput(DipoleInputField);
  auto bbox = fh->vmesh()->get_bounding_box();
  bool is_vector = (pointWidgets_[id]->size() == 4);

  double scale = scale_[id];
  if(state->getValue(Sizing).toInt() == SizingType::NORMALIZE_BY_LARGEST_VECTOR)
    scale /= state->getValue(LargestSize).toDouble();

  // createDipoleWidget(bbox, pos_[id], direction_[id], scale * state->getValue(WidgetScaleFactor).toDouble(), id, is_vector);
  ArrowWidget arrow(
      *this, "SAED", scale * state->getValue(WidgetScaleFactor).toDouble(),
      pos_[id], direction_[id], resolution_, is_vector,
      id, ++widgetIter_, bbox);
}

void ShowAndEditDipoles::ReceiveInputPoints()
{
  FieldHandle fh = getRequiredInput(DipoleInputField);
  VField* vf = fh->vfield();
  Point p;
  pos_.clear();
  for(const auto& node : fh->mesh()->getFacade()->nodes())
  {
    size_t index = node.index();
    vf->get_center(p, index);
    pos_.push_back(p);
  }
}

void ShowAndEditDipoles::ReceiveInputDirections()
{
  FieldHandle fh = getRequiredInput(DipoleInputField);
  VField* vf = fh->vfield();
  Vector v;
  direction_.clear();
  for(const auto& node : fh->mesh()->getFacade()->nodes())
  {
    size_t index = node.index();
    vf->get_value(v, index);
    direction_.push_back(v.normal());
  }
}

void ShowAndEditDipoles::ReceiveInputScales()
{
  FieldHandle fh = getRequiredInput(DipoleInputField);
  VField* vf = fh->vfield();
  auto state = get_state();
  Vector v;
  scale_.clear();
  double newLargest = 0.0;
  for(const auto& node : fh->mesh()->getFacade()->nodes())
  {
    size_t index = node.index();
    vf->get_value(v, index);
    scale_.push_back(v.length());
    newLargest = std::max(newLargest, std::abs(v.length()));
  }
  state->setValue(LargestSize, newLargest);
}

void ShowAndEditDipoles::makeScalesPositive()
{
  // If dipole is scaled below 0, make the scale positive and flip the direction
  for(size_t i = 0; i < scale_.size(); i++)
  {
    if(scale_[i] < 0.0)
    {
      scale_[i] = std::abs(scale_[i]);
      direction_[i] = -direction_[i];
    }
  }
}

void ShowAndEditDipoles::ReceiveInputField()
{
  FieldHandle fh = getRequiredInput(DipoleInputField);
  VField* vf = fh->vfield();
  auto state = get_state();
  Point p;
  Vector v;
  direction_.clear();
  pos_.clear();
  scale_.clear();
  double newLargest = 0.0;
  for(const auto& node : fh->mesh()->getFacade()->nodes())
  {
    size_t index = node.index();
    vf->get_center(p, index);
    vf->get_value(v, index);
    pos_.push_back(p);
    direction_.push_back(v.normal());
    scale_.push_back(v.length());
    newLargest = std::max(newLargest, std::abs(v.length()));
  }
  state->setValue(LargestSize, newLargest);
}

void ShowAndEditDipoles::GenerateOutputGeom()
{
  FieldHandle fh = getRequiredInput(DipoleInputField);
  auto state = get_state();
  auto bbox = fh->vmesh()->get_bounding_box();

  last_bounds_ = bbox;
  pointWidgets_.resize(pos_.size());

  // Create all but last dipole as vector
  for(size_t i = 0; i < pos_.size() - 1; i++)
  {
    double scale = scale_[i];
    if(state->getValue(Sizing).toInt() == SizingType::NORMALIZE_BY_LARGEST_VECTOR)
      scale /= state->getValue(LargestSize).toDouble();

    // createDipoleWidget(bbox, pos_[i], direction_[i], scale * state->getValue(WidgetScaleFactor).toDouble(), i, true);
    ArrowWidget arrow(
        *this, "SAED", scale * state->getValue(WidgetScaleFactor).toDouble(),
        pos_[i], direction_[i], resolution_, true, i, ++widgetIter_,
        bbox);
  }

  // Create last dipoles separately to check if shown as vector
  size_t last_id = pos_.size() - 1;

  double scale = scale_[last_id];
  if(state->getValue(Sizing).toInt() == SizingType::NORMALIZE_BY_LARGEST_VECTOR)
    scale /= state->getValue(LargestSize).toDouble();

  // createDipoleWidget(bbox, pos_[last_id], direction_[last_id], scale * state->getValue(WidgetScaleFactor).toDouble(), last_id, state->getValue(ShowLastAsVector).toBool());
  ArrowWidget(
      *this, "SAED", scale * state->getValue(WidgetScaleFactor).toDouble(),
      pos_[last_id], direction_[last_id], resolution_,
      state->getValue(ShowLastAsVector).toBool(), last_id, ++widgetIter_, bbox);

  lastVectorShown_ = state->getValue(ShowLastAsVector).toBool();
}


GeometryHandle ShowAndEditDipoles::addLines()
{
  FieldHandle fh = getRequiredInput(DipoleInputField);
  auto state = get_state();
  auto bbox = fh->vmesh()->get_bounding_box();

  SpireIBO::PRIMITIVE primIn = SpireIBO::PRIMITIVE::LINES;
  std::string idName = "SAEDField" +
    GeometryObject::delimiter +
    state->getValue(ShowAndEditDipoles::FieldName).toString()
    + " (from " + id().id_ +")" +
    "(" + std::to_string(widgetIter_) + ")";

  auto geom(boost::make_shared<GeometryObjectSpire>(*this, idName, true));
  GlyphGeom glyphs;

  RenderState renState;
  renState.set(RenderState::USE_NORMALS, true);
  renState.set(RenderState::IS_ON, true);
  renState.set(RenderState::USE_TRANSPARENT_EDGES, false);
  renState.mGlyphType = RenderState::GlyphType::LINE_GLYPH;
  renState.defaultColor = lineCol_;
  renState.set(RenderState::USE_DEFAULT_COLOR, true);

  // Create lines between every point
  for(size_t a = 0; a < pos_.size(); a++)
  {
    for(size_t b = a + 1; b < pos_.size(); b++)
    {
      glyphs.addLine(pos_[a], pos_[b], lineCol_, lineCol_);
    }
  }

  glyphs.buildObject(*geom, idName, false, 0.5, ColorScheme::COLOR_UNIFORM, renState, primIn, bbox);
  return geom;
}

FieldHandle ShowAndEditDipoles::makePointCloud()
{
  FieldInformation fi("PointCloudMesh", 0, "Vector");
  auto ofield = CreateField(fi);
  auto mesh = ofield->vmesh();
  auto field = ofield->vfield();
  auto state = get_state();

  for (size_t i = 0; i < pointWidgets_.size(); i++)
  {
    VMesh::Node::index_type pcindex = mesh->add_point(pos_[i]);
    field->resize_fdata();

    double scale = scale_[i];
    if(state->getValue(Sizing).toInt() == SizingType::NORMALIZE_BY_LARGEST_VECTOR)
      scale /= state->getValue(LargestSize).toDouble();

    field->set_value(static_cast<Vector>(direction_[i] * scale), pcindex);
  }
  return ofield;
}

const AlgorithmParameterName ShowAndEditDipoles::FieldName("FieldName");
const AlgorithmParameterName ShowAndEditDipoles::WidgetScaleFactor("WidgetScaleFactor");
const AlgorithmParameterName ShowAndEditDipoles::Sizing("Sizing");
const AlgorithmParameterName ShowAndEditDipoles::ShowLastAsVector("ShowLastAsVector");
const AlgorithmParameterName ShowAndEditDipoles::ShowLines("ShowLines");
const AlgorithmParameterName ShowAndEditDipoles::Reset("Reset");
const AlgorithmParameterName ShowAndEditDipoles::MoveDipolesTogether("MoveDipolesTogether");
const AlgorithmParameterName ShowAndEditDipoles::DipolePositions("DipolePositions");
const AlgorithmParameterName ShowAndEditDipoles::DipoleDirections("DipoleDirections");
const AlgorithmParameterName ShowAndEditDipoles::DipoleScales("DipoleScales");
const AlgorithmParameterName ShowAndEditDipoles::DataSaved("DataSaved");
const AlgorithmParameterName ShowAndEditDipoles::LargestSize("LargestSize");
