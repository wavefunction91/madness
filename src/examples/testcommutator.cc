/*
  This file is part of MADNESS.

  Copyright (C) 2007,2010 Oak Ridge National Laboratory

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

  For more information please contact:

  Robert J. Harrison
  Oak Ridge National Laboratory
  One Bethel Valley Road
  P.O. Box 2008, MS-6367

  email: harrisonrj@ornl.gov
  tel:   865-241-3937
  fax:   865-572-0680


  $Id$
*/

/// \file testbsh.cc
/// \brief test the bsh operator

#include <madness/mra/mra.h>
#include <madness/mra/operator.h>
#include <madness/constants.h>

using namespace madness;

template <typename T, std::size_t NDIM>
class Gaussian : public FunctionFunctorInterface<T,NDIM> {
public:
    typedef Vector<double,NDIM> coordT;
    const coordT center;
    const double exponent;
    const T coefficient;

    Gaussian(const coordT& center, double exponent, T coefficient)
            : center(center), exponent(exponent), coefficient(coefficient) {};

    T operator()(const coordT& x) const {
        double sum = 0.0;
        for (std::size_t i=0; i<NDIM; ++i) {
            double xx = center[i]-x[i];
            sum += xx*xx;
        };
        return coefficient*exp(-exponent*sum);
    };
};


double aa;

double q(double r) {
    double val;
    if (r < 0.1e-4)
        val = (0.2e1 * exp(0.1e1 / aa / 0.4e1) * exp(-0.1e1 / aa / 0.4e1) * sqrt(aa) / sqrt(constants::pi) + exp(0.1e1 / aa / 0.4e1) * erf(0.1e1 / sqrt(aa) / 0.2e1) - exp(0.1e1 / aa / 0.4e1) + (0.2e1 / 0.3e1 * exp(0.1e1 / aa / 0.4e1) * exp(-0.1e1 / aa / 0.4e1) * (0.1e1 / 0.2e1 - aa) * sqrt(aa) / sqrt(constants::pi) + exp(0.1e1 / aa / 0.4e1) * erf(0.1e1 / sqrt(aa) / 0.2e1) / 0.6e1 - exp(0.1e1 / aa / 0.4e1) / 0.6e1) * r * r);
    else
        val = ((-exp((0.1e1 + 0.4e1 * aa * r) / aa / 0.4e1) + exp(-(-0.1e1 + 0.4e1 * aa * r) / aa / 0.4e1) + exp((0.1e1 + 0.4e1 * aa * r) / aa / 0.4e1) * erf((0.2e1 * aa * r + 0.1e1) / sqrt(aa) / 0.2e1) + exp(-(-0.1e1 + 0.4e1 * aa * r) / aa / 0.4e1) * erf((-0.1e1 + 0.2e1 * aa * r) / sqrt(aa) / 0.2e1)) / r / 0.2e1);

    return val / (4.0*constants::pi);
}


/// the result of the convolution
struct Qfunc : public FunctionFunctorInterface<double,3> {
    double operator()(const Vector<double,3>& x) const {
        double r = sqrt(x[0]*x[0] + x[1]*x[1] + x[2]*x[2]);
        return q(r);
    }
};

template <typename T>
void test_bsh(World& world) {

    // Setting up the derivative types
    std::vector<const char*> DATA_PATH;
    DATA_PATH.push_back("/gpfs/projects/rjh/mad-der/src/madness/mra/b-spline-deriv1.txt");
    DATA_PATH.push_back("/gpfs/projects/rjh/mad-der/src/madness/mra/b-spline-deriv2.txt");
    DATA_PATH.push_back("/gpfs/projects/rjh/mad-der/src/madness/mra/b-spline-deriv3.txt");
    DATA_PATH.push_back("/gpfs/projects/rjh/mad-der/src/madness/mra/ble-first.txt");
    DATA_PATH.push_back("/gpfs/projects/rjh/mad-der/src/madness/mra/ble-second.txt");

    double mu = 1.0;
    std::vector<long> npt(3,201l);
    typedef Vector<double,3> coordT;
    typedef std::shared_ptr< FunctionFunctorInterface<T,3> > functorT;

    FunctionDefaults<3>::set_cubic_cell(-100,100);
    FunctionDefaults<3>::set_thresh(1e-6);
    FunctionDefaults<3>::set_refine(true);
    FunctionDefaults<3>::set_autorefine(true);
    FunctionDefaults<3>::set_truncate_mode(1);
    FunctionDefaults<3>::set_truncate_on_project(true);

    // Output headers formated nicely
    printf("\n%4s %16s %20s %20s %20s %20s %20s\n", "k", "deriv. type", "d/dx(op(f)) norm", "d/dx(op(f)) error", "op(d/dx(f)) norm", "op(d/dx(f)) error", "norm of dif.");
    printf("---- ---------------- -------------------- -------------------- -------------------- -------------------- --------------------\n");

    for (int i=0; i<=5; i++) {

       // Choose derivative type
       std::string deriv_type;
       if (i == 0) deriv_type = "abgv";
       else if (i == 1) deriv_type = "bspline1";
       else if (i == 2) deriv_type = "bspline2";
       else if (i == 3) deriv_type = "bspline3";
       else if (i == 4) deriv_type = "ble1";
       else if (i == 5) deriv_type = "ble2";
      
       // Only do first derivatives
       if(i == 2 or i == 3 or i == 5) continue;
 
       // Prolates are only available for k=8+, so taking care of that here
       for (int k=7 ; k<=15; k++) {
           FunctionDefaults<3>::set_k(k);

           FunctionDefaults<3>::set_initial_level(6);
           if (k >= 6) FunctionDefaults<3>::set_initial_level(5);

           // Now construct the derivative
           std::vector< std::shared_ptr<real_derivative_3d> > diff = gradient_operator<double,3>(world);
           
           // Change derivative to correct type
           if (deriv_type == "bspline1") {
              for(int j=0; j<3; j++) (*diff[j]).read_from_file(DATA_PATH[0]);
           }
           else if (deriv_type == "bspline2") {
              for(int j=0; j<3; j++) (*diff[j]).read_from_file(DATA_PATH[1], 2);
           }
           else if (deriv_type == "bspline3") {
              for(int j=0; j<3; j++) (*diff[j]).read_from_file(DATA_PATH[2], 3);
           }
           else if (deriv_type == "ble1") {
              for(int j=0; j<3; j++) (*diff[j]).read_from_file(DATA_PATH[3]);
           }
           else if (deriv_type == "ble2") {
              for(int j=0; j<3; j++) (*diff[j]).read_from_file(DATA_PATH[4], 2);
           }
           const coordT origin(0.0);
           const double expnt = 100.0;
           aa = expnt;
           const double coeff = pow(expnt/constants::pi,1.5);
           
           // the input function to be convolved
           Function<T,3> f = FunctionFactory<T,3>(world).functor(functorT(new Gaussian<T,3>(origin, expnt, coeff)));
           f.truncate();

           SeparatedConvolution<T,3> op = BSHOperator<3>(world, mu, 1e-4, 1e-8);
           std::cout.precision(8);

           // apply the convolution operator on the input function f
           Function<T,3> ff = copy(f);
           Function<T,3> opf = op(ff);
           
           ff.clear();
           opf.verify_tree();
           
           Function<T,3> opinvopf = opf*(mu*mu);
           for (int axis=0; axis<3; ++axis) {
               opinvopf.gaxpy(1.0, (*diff[axis])( (*diff[axis])( opf ) ),-1.0);
           }

           double opinvof_norm = opinvopf.norm2();
           Function<T,3> error = (f-opinvopf);
           double error_norm = error.norm2();
           
           Function<T,3> g = (mu*mu)*f;
           for (int axis=0; axis<3; ++axis) {
               g = g - (*diff[axis])((*diff[axis])(f.reconstruct()).reconstruct());
           }
           g = op(g);
           double derror=(g-f).norm2();
           double g_norm = g.norm2();
           double com_norm = (opinvopf-g).norm2();          
 
           // Output data of interest
           printf("%4d %16s %20.8f %20.8f %20.8f %20.8f %20.8f\n", k, deriv_type.c_str(), opinvof_norm, error_norm, g_norm, derror, com_norm);
       }
    }
        
    world.gop.fence();
}


int main(int argc, char**argv) {
    initialize(argc,argv);
    World world(SafeMPI::COMM_WORLD);

    try {
        startup(world,argc,argv);

        test_bsh<double>(world);

    }
    catch (const SafeMPI::Exception& e) {
        print(e);
        error("caught an MPI exception");
    }
    catch (const madness::MadnessException& e) {
        print(e);
        error("caught a MADNESS exception");
    }
    catch (const madness::TensorException& e) {
        print(e);
        error("caught a Tensor exception");
    }
    catch (char* s) {
        print(s);
        error("caught a c-string exception");
    }
    catch (const char* s) {
        print(s);
        error("caught a c-string exception");
    }
    catch (const std::string& s) {
        print(s);
        error("caught a string (class) exception");
    }
    catch (const std::exception& e) {
        print(e.what());
        error("caught an STL exception");
    }
    catch (...) {
        error("caught unhandled exception");
    }

    world.gop.fence();
    finalize();

    return 0;
}

