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


#ifndef INTERFACE_APPLICATION_PYTHONCONSOLEWIDGET_H
#define INTERFACE_APPLICATION_PYTHONCONSOLEWIDGET_H

#include <boost/shared_ptr.hpp>
#include <QDockWidget>

#ifdef BUILD_WITH_PYTHON
class PythonConsoleWidgetPrivate;
typedef boost::shared_ptr< PythonConsoleWidgetPrivate > PythonConsoleWidgetPrivateHandle;

namespace SCIRun
{
namespace Gui
{

class PythonConsoleWidget : public QDockWidget
{
	Q_OBJECT

public:
  explicit PythonConsoleWidget(class NetworkEditor* rootNetworkEditor, QWidget* parent = nullptr);
	virtual ~PythonConsoleWidget();

	void runWizardCommand(const QString& code);

public Q_SLOTS:
  void showBanner();

private:
	PythonConsoleWidgetPrivateHandle private_;
};

}}

#else
class PythonConsoleWidget {};
#endif

#endif
