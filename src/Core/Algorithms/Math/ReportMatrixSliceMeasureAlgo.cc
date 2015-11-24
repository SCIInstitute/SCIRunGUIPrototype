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

#include <Core/Algorithms/Math/ReportMatrixSliceMeasureAlgo.h>


#include <sstream>

using namespace SCIRun;
using namespace SCIRun::Core::Datatypes;
using namespace SCIRun::Core::Algorithms;
using namespace SCIRun::Core::Algorithms::Math;

ReportMatrixSliceMeasureAlgo::ReportMatrixSliceMeasureAlgo()
{
  addParameter(Variables::Operator , 0);
  addParameter(Variables::Method, 0);
}


AlgorithmOutput ReportMatrixSliceMeasureAlgo::run_generic(const AlgorithmInput& input) const
{
  
  auto input_matrix = input.get<Matrix>(Variables::InputMatrix);
  
  MatrixHandle return_matrix;
  
  auto op = get(Variables::Operator).toInt();
  auto method = get(Variables::Method).toInt();
  
  switch (op)
  {
    case 0:
      ApplyRowOperation(input_matrix,return_matrix, method);
      break;
    case 1:
      ApplyColumnOperation(input_matrix,return_matrix, method);
      break;
  }
  
  AlgorithmOutput output;
  output[Variables::OutputMatrix] = return_matrix;
  return output;
  
}


bool
ReportMatrixSliceMeasureAlgo::ApplyRowOperation(MatrixHandle input, MatrixHandle& output,int method)
{
  
  if (input)
  {
    error("ApplyRowOperation: no input matrix found");
    return false;
  }
  
  
  
  size_type nrows = input->nrows();
  size_type ncols = input->ncols();
  
  output = new DenseMatrix(nrows, 1);
  auto omat = matrix_cast::as_dense ( output)
  
  double *dest = omat->data();
  
  for (index_type q=0; q<nrows; q++) dest[q] = 0.0;
  
  if (output)
  {
    error("ApplyRowOperation: could not create output matrix");
    return false;  
  }

  
/*
  if (matrix_is::sparse(input))
  {
    index_type *rows = input->sparse()->get_rows();
    double *vals = input->sparse()->get_vals();
    
    if (method == "Sum")
    {
      for (index_type q=0; q<nrows; q++) { for (index_type r = rows[q]; r < rows[q+1]; r++) dest[q] += vals[r]; }        
    }
    else if ((method == "Average")||(method == "Mean"))
    {
      for (index_type q=0; q<nrows; q++) 
      {
        for (index_type r = rows[q]; r < rows[q+1]; r++) dest[q] += vals[r];  
        dest[q] /= static_cast<double>(ncols);  
      }            
    }
    else if (method == "Norm")
    {
      for (index_type q=0; q<nrows; q++) 
      {
        for (index_type r = rows[q]; r < rows[q+1]; r++) dest[q] += (vals[r])*(vals[r]);  
        if (rows[q] != rows[q+1]) dest[q] = sqrt(dest[q]);  
      }                
    }
    else if (method == "Variance")
    {
      double mean;
      for (index_type q=0; q<nrows; q++) 
      {
        mean = 0.0;
        for (index_type r = rows[q]; r < rows[q+1]; r++) mean += vals[r];
        if (ncols) mean /= static_cast<double>(ncols);
        for (index_type r = rows[q]; r < rows[q+1]; r++) dest[q] += (vals[r]-mean)*(vals[r]-mean);
        dest[q] += (ncols-(rows[q+1]-rows[q]))*mean*mean;  
        if (ncols > 1) dest[q] = dest[q]/static_cast<double>(ncols-1); else dest[q] = 0.0;
      }               
    }
    else if (method == "StdDev")
    {
      double mean;
      for (index_type q=0; q<nrows; q++) 
      {
        mean = 0.0;
        for (index_type r = rows[q]; r < rows[q+1]; r++) mean += vals[r];
        if (ncols) mean /= static_cast<double>(ncols);
        for (index_type r = rows[q]; r < rows[q+1]; r++) dest[q] += (vals[r]-mean)*(vals[r]-mean);  
        dest[q] += (ncols-(rows[q+1]-rows[q]))*mean*mean;  
        if (ncols > 1) dest[q] = sqrt(dest[q]/static_cast<double>(ncols-1)); else dest[q] = 0.0;
      }               
    }
    else if (method == "Maximum")
    {
      for (index_type q=0; q<nrows; q++) 
      {
        if (rows[q+1]-rows[q] == ncols) dest[q] = -DBL_MAX; else dest[q] = 0.0;
        for (index_type r = rows[q]; r < rows[q+1]; r++) if (vals[r] > dest[q]) dest[q] = vals[r];
      }                   
    }
    else if (method == "Minimum")
    {
      for (index_type q=0; q<nrows; q++) 
      {
        if (rows[q+1]-rows[q] == ncols) dest[q] = DBL_MAX; else dest[q] = 0.0;
        for (index_type r = rows[q]; r < rows[q+1]; r++) if (vals[r] < dest[q]) dest[q] = vals[r];
      }                   
    }
    else if (method == "Median")
    {
      for (index_type q=0; q<nrows; q++) 
      {
        std::vector<double> dpos;
        std::vector<double> dneg;
        for (index_type r = rows[q]; r < rows[q+1]; r++) if (vals[r] < 0.0 ) dneg.push_back(vals[r]); else dpos.push_back(vals[r]);
        if (dpos.size() >= static_cast<size_t>(ncols/2))
          std::sort(dpos.begin(),dpos.end());
        if (dneg.size() >= static_cast<size_t>(ncols/2))
          std::sort(dneg.begin(),dneg.end());
        if (2*(ncols/2) == ncols && ncols)
        {
          double val1 = 0.0;
          double val2 = 0.0;
          size_type q1 = (ncols/2)-1;
          size_type q2 = ncols/2; 
          if ( q1 < static_cast<size_type>(dneg.size())) val1 = dneg[q1];
          if ( q2 < static_cast<size_type>(dneg.size())) val2 = dneg[q2];
          if ( (ncols-1)-q1 < static_cast<size_type>(dpos.size())) 
              val1 = dpos[dpos.size()-ncols+q1];
          if ( (ncols-1)-q2 < static_cast<size_type>(dpos.size())) 
              val2 = dpos[dpos.size()-ncols+q2];          
          dest[q] = 0.5*(val1+val2);
        }
        else
        {
          double val1 = 0.0;
           size_type q1 = (ncols/2);
          if ( q1 < static_cast<size_type>(dneg.size())) val1 = dneg[q1];
          if ( (ncols-1)-q1 < static_cast<size_type>(dpos.size()))
            val1 = dpos[dpos.size()-ncols+q1];
          dest[q] = val1;
        }       
      }                   
    }
    else
    {
      error ("ApplyRowOperation: This method has not yet been implemented");
      return false;
    }
  }*/
  
  if (!matrix_is::dense(input))
  {
    //TODO implement something with sparse
    error("Currently only works with dense matrices");
    return;
  }
  else
  {
    auto mat  = matrix_cast::as_dense (input);
    double* data = mat ->data()
    //DenseMatrix* mat = input->dense();
    
    size_type m = mat->nrows();
    size_type n = mat->ncols();
    //double* data = mat->get_data_pointer();
  
    if (method == "Sum")
    {
      index_type k = 0;
      for (index_type p=0; p<m; p++) dest[p] = 0.0;
      for (index_type p=0; p<m; p++) 
        for (index_type q=0; q<n; q++) dest[p] += data[k++]; 
    }
    else if (method == "Mean")
    {
      index_type k = 0;
      for (index_type p=0; p<m; p++) dest[p] = 0.0;
      for (index_type p=0; p<m; p++) 
        for (index_type q=0; q<n; q++) dest[p] += data[k++]; 
      for (index_type p=0; p<m; p++) dest[p] /= static_cast<double>(n);  
    }
    else if (method == "Variance")
    {
      std::vector<double> mean(m);
      index_type k = 0;
      for (index_type p=0; p<m; p++) dest[p] = 0.0;
      for (index_type p=0; p<m; p++) 
        for (index_type q=0; q<n; q++) mean[p] += data[k++];
      for (index_type p=0; p<m; p++) mean[p] /= static_cast<double>(n);  
      k = 0;
      for (index_type p=0; p<m; p++) 
        for (index_type q=0; q<n; q++, k++) 
          dest[p] += (data[k]-mean[p])*(data[k]-mean[p]); 
      for (index_type p=0; p<m; p++)  
        if (n > 1) dest[p] /= static_cast<double>(n-1);  else dest[p] = 0.0;
    }
    else if (method == "StdDev")
    {
      std::vector<double> mean(m);
      index_type k = 0;
      for (index_type p=0; p<m; p++) dest[p] = 0.0;
      for (index_type p=0; p<m; p++) 
        for (index_type q=0; q<n; q++) mean[p] += data[k++];
      for (index_type p=0; p<m; p++) mean[p] /= static_cast<double>(n);  
      k = 0;
      for (index_type p=0; p<m; p++) 
        for (index_type q=0; q<n; q++, k++) dest[p] += (data[k]-mean[p])*(data[k]-mean[p]); 
      for (index_type p=0; p<m; p++)  
        if (n > 1) dest[p] = sqrt(dest[p]/static_cast<double>(n-1));  
        else dest[p] = 0.0;
    }
    else if (method == "Norm")
    {
      index_type k = 0;
      for (index_type p=0; p<m; p++) dest[p] = 0.0;
      for (index_type p=0; p<m; p++) 
        for (index_type q=0; q<n; q++, k++) dest[p] += data[k]*data[k]; 
      for (index_type p=0; p<m; p++) dest[p] = sqrt(dest[p]); 
    }
    else if (method == "Maximum")
    {
      index_type k = 0;
      for (index_type p=0; p<m; p++) dest[p] = -DBL_MAX;
      for (index_type p=0; p<m; p++) 
        for (index_type q=0; q<n; q++, k++) 
          if (data[k] > dest[p]) dest[p] = data[k]; 
    }
    else if (method == "Minimum")
    {
      index_type k = 0;
      for (index_type p=0; p<m; p++) dest[p] = DBL_MAX;
      for (index_type p=0; p<m; p++) 
        for (index_type q=0; q<n; q++, k++) 
          if (data[k] < dest[p]) dest[p] = data[k]; 
    }
    else if (method == "Median")
    {
      index_type k = 0;
      std::vector<double> v(n);

      for (index_type p=0; p<m; p++)
      {
        for (index_type q=0; q<n; q++, k++) v[q] = data[k];
        std::sort(v.begin(),v.end());
        if ((n/2)*2 == n)
        {
          dest[p] = 0.5*(v[n/2]+v[(n/2) -1]);
        }
        else
        {
          dest[p] = v[n/2];
        }
      }
    }
    else
    {
      error("ApplyRowOperation: This method has not yet been implemented");
      return false;    
    }
  }
  
  
  return true;
}


bool
ReportMatrixSliceMeasureAlgo::ApplyColumnOperation(MatrixHandle input, MatrixHandle& output, int method)
{
  
  if (input)
  {
    error("ApplyRowOperation: no input matrix found");
    return false;
  }
  MatrixHandle t = input->make_transpose();
  if(!(ApplyRowOperation(t,t,method))) return false;
  output = t->make_transpose();
  
  return true;
} 
