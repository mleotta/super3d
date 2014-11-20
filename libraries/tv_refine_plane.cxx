/*
* Copyright 2012 Kitware, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of this project nor the names of its contributors
 *       may be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "tv_refine_plane.h"

#include <sstream>
#include <iomanip>

#include <vil/vil_convert.h>
#include <vil/vil_save.h>
#include <vil/algo/vil_median.h>
#include <vil/vil_math.h>

#include <vcl_vector.h>

#include <vnl/vnl_double_3.h>
#include <vnl/vnl_double_2.h>
#include <video_transforms/dual_rof_denoise.h>

#include "tv_utils.h"
#include "config.h"
#include "meanshift_normals.h"
#include "rof_normals.h"

void
refine_depth_planar(world_space *ws,
                    vil_image_view<double> &d,
                    vil_image_view<double> &g,
                    const vil_image_view<double> &ref,
                    double lambda,
                    double step)
{
  vil_image_view<double> q(d.ni(), d.nj(), 2);
  q.fill(0.0);
  vil_image_view<double> a(d.ni(), d.nj(), 1);
  a.deep_copy(d);
  vil_image_view<double> n(d.ni(), d.nj(), 3);

  const unsigned int max_iter = 5;
  vil_image_view<double> last_depth;
  last_depth.deep_copy(d);

  double normal_bandwidth = 0.4;
  double radius = 10.0;
  double depth_bandwidth = 0.05;
  double intensity_bandwidth = 0.05;
  double epsilon = config::inst()->get_value<double>("epsilon");

  //g.fill(1.0);
  vil_image_view<double> bp(d.ni(), d.nj(), 3);
  //vil_image_view<double> n_new(d.ni(), d.nj(), 3);
  for (unsigned int iter = 0; iter < max_iter; iter++)
  {
    //vidtk::dual_rof_weighted_denoise(vil_plane<double>(n,0), g, vil_plane(n_new,0), 1, 1);
    //vidtk::dual_rof_weighted_denoise(vil_plane<double>(n,1), g, vil_plane(n_new,1), 1, 1);
    //vidtk::dual_rof_weighted_denoise(vil_plane<double>(n,2), g, vil_plane(n_new,2), 1, 1);
    //n.deep_copy(n_new);

    //compute_normals_eig(d, bp, n, ws, 1, 0.5);
    //meanshift(n, d, ref, ws, radius, depth_bandwidth, normal_bandwidth, intensity_bandwidth);

    normals_rof(n, d, bp, ws, g, 100, 2, 1e2, 0.25, 0.01);

    //vil_plane<double>(n,0).fill(0.0);
    //vil_plane<double>(n,1).fill(0.0);
    //vil_plane<double>(n,2).fill(0.1);


    double ssd;
    do
    {
      huber_planar_rof(q, d, g, a, n, lambda, step, epsilon);
      ssd = vil_math_ssd(d,last_depth,double());
      last_depth.deep_copy(d);
      vcl_cout << "SSD: " << ssd << "\n";
    } while (ssd > 1e-4);
  }
}

void refine_depth_planar(const vil_image_view<double> &cost_volume,
                         world_space *ws,
                         vil_image_view<double> &d,
                         const vil_image_view<double> &g,
                         const vil_image_view<double> &ref,
                         double beta,
                         double theta0,
                         double theta_end,
                         double lambda)
{
  vil_image_view<double> sqrt_cost_range(cost_volume.ni(), cost_volume.nj(), 1);
  double a_step = 1.0 / cost_volume.nplanes();

  for (unsigned int j = 0; j < cost_volume.nj(); j++)
  {
    for (unsigned int i = 0; i < cost_volume.ni(); i++)
    {
      double min, max;
      min = max = cost_volume(i,j,0);
      unsigned int min_k = 0;
      for (unsigned int k = 1; k < cost_volume.nplanes(); k++)
      {
        const double &cost = cost_volume(i,j,k);
        if (cost < min) {
          min = cost;
          min_k = k;
        }
        if (cost > max)
          max = cost;
      }
      sqrt_cost_range(i,j) = vcl_sqrt(max - min);
    }
  }

  vil_image_view<double> q(cost_volume.ni(), cost_volume.nj(), 2);
  q.fill(0.0);
  vil_image_view<double> a(cost_volume.ni(), cost_volume.nj(), 1);

  vil_image_view<double> n(cost_volume.ni(), cost_volume.nj(), 3);
  vil_plane<double>(n, 0).fill(0.0);
  vil_plane<double>(n, 1).fill(0.0);
  vil_plane<double>(n, 2).fill(1.0);

  unsigned int max_iter = 1;
  double denom = log(10.0);
  vil_image_view<double> last_depth;
  last_depth.deep_copy(d);

  double normal_bandwidth = 0.4;
  double radius = 10.0;
  double depth_bandwidth = 0.05;
  double image_bandwidth = 0.05;

  double new_lambda = lambda/100.0;
  double epsilon = config::inst()->get_value<double>("epsilon");
  lambda = new_lambda;
  vil_image_view<double> bp(d.ni(), d.nj(), 3);

  for (unsigned int iter = 0; iter < max_iter; iter++)
  {
    //compute_normals_tri(d, n, ws);
    //compute_normals_eig(d, bp, n, ws, 1, 0.5);
    //meanshift(n, d, ref, ws, radius, depth_bandwidth, normal_bandwidth, image_bandwidth);
    normals_rof(n, d, bp, ws, g, 1000, 1, 5e2, 0.25, 0.01);

    vcl_cout << "Lambda: " << lambda << "\n";
    double theta = theta0;
    while (theta > theta_end)
    {
      vcl_cout << iter << " theta: " << theta << "\n";
      min_search_bound(a, d, cost_volume, sqrt_cost_range, theta, lambda);
      huber_planar_coupled(q, d, g, a, n, theta, 0.25/theta, epsilon);
      theta = pow(10.0, log(theta)/denom - beta);
    }
  }
}

//semi-implicit gradient ascent on q and descent on d
void
huber_planar_rof(vil_image_view<double> &q,
           vil_image_view<double> &d,
           const vil_image_view<double> &g,
           const vil_image_view<double> &a,
           const vil_image_view<double> &n,
           double lambda,
           double step,
           double epsilon)
{
  unsigned int ni = d.ni() - 1, nj = d.nj() - 1;
  double stepsilon1 = 1.0 + step*epsilon;
  for (unsigned int j = 0; j < nj; j++)
  {
    for (unsigned int i = 0; i < ni; i++)
    {
      double &qx = q(i,j,0), &qy = q(i,j,1);
      double dij = d(i,j);
      qx = (qx + step * g(i,j) * (d(i+1,j) + n(i,j,0)/n(i,j,2) - dij))/stepsilon1;
      qy = (qy + step * g(i,j) * (d(i,j+1) + n(i,j,1)/n(i,j,2) - dij))/stepsilon1;

      //truncate vectors
      double mag = qx*qx + qy*qy;
      if (mag > 1.0f)
      {
        mag = sqrt(mag);
        qx /= mag;
        qy /= mag;
      }
    }
  }

  double denom = (1.0 + step * lambda);
  for (unsigned int j = 0; j < d.nj(); j++)
  {
    for (unsigned int i = 0; i < d.ni(); i++)
    {
      //add scaled divergence
      double divx = q(i,j,0), divy = q(i,j,1);
      if (i > 0)  divx -=  q(i-1,j,0);
      if (j > 0)  divy -=  q(i,j-1,1);

      double &dij = d(i,j);
      dij = (dij + step * (g(i,j) * (divx + divy) + lambda * a(i,j)))/denom;
    }
  }
}

//semi-implicit gradient ascent on q and descent on d
void
huber_planar_coupled(vil_image_view<double> &q,
           vil_image_view<double> &d,
           const vil_image_view<double> &g,
           const vil_image_view<double> &a,
           const vil_image_view<double> &n,
           double theta,
           double step,
           double epsilon)
{
  unsigned int ni = d.ni() - 1, nj = d.nj() - 1;
  double stepsilon1 = 1.0 + step*epsilon;
  for (unsigned int j = 0; j < nj; j++)
  {
    for (unsigned int i = 0; i < ni; i++)
    {
      double &qx = q(i,j,0), &qy = q(i,j,1);
      double dij = d(i,j);
      qx = (qx + step * g(i,j) * (d(i+1,j) + n(i+1,j,0)/n(i+1,j,2) - dij))/stepsilon1;
      qy = (qy + step * g(i,j) * (d(i,j+1) + n(i,j+1,1)/n(i,j+1,2) - dij))/stepsilon1;

      //truncate vectors
      double mag = qx*qx + qy*qy;
      if (mag > 1.0f)
      {
        mag = sqrt(mag);
        qx /= mag;
        qy /= mag;
      }
    }
  }

  double theta_inv = 1.0 / theta, denom = (1.0 + (step / theta));
  for (int j = 0; j < d.nj(); j++)
  {
    for (int i = 0; i < d.ni(); i++)
    {
      //add scaled divergence
      double divx = q(i,j,0), divy = q(i,j,1);
      if (i > 0)  divx -=  q(i-1,j,0);
      if (j > 0)  divy -=  q(i,j-1,1);

      double &dij = d(i,j);
      dij = (dij + step * (g(i,j) * (divx + divy) + theta_inv * a(i,j)))/denom;
    }
  }
}
