#ifndef MRA_FUNCPLOT_H
#define MRA_FUNCPLOT_H

// No need to include this file yourself ... it is included in mra.h

    /// Writes an OpenDX format file with a cube/slice of points on a uniform grid

    /// Collective operation but only process 0 writes the file.  By convention OpenDX
    /// files end in ".dx" but this choice is up to the user.  The binary format is
    /// more compact and vastly faster to both write and load but is not as portable.
    ///
    /// Now follow some brief tips about how to look at files inside OpenDX.
    ///
    /// To view a 1D function \c file-selector-->import-->plot-->image.
    ///
    /// To view a 2D function as a colored plane \c file-selector-->import-->autocolor-->image.
    ///
    /// To view a 2D function as a 3D surface \c file-selector-->import-->rubbersheet-->image.
    ///
    /// To view a 3D function as an isosurface \c file-selector-->import-->isosurface-->image.
    ///
    /// To select the real/imaginary/absolute value of a complex number insert a compute
    /// element after the import.
    template <typename T, int NDIM>
    void plotdx(const Function<T,NDIM>& f,
                const char* filename,
                const Tensor<double>& cell,
                const std::vector<long>& npt,
                bool binary=true);


    /// Writes header information for plotting from a VTK file (i.e. in ParaView).
    ///
    /// VTK plotting is only currently supported for
    /// 1) 3-D functions
    /// 2) Real-valued madness::function objects
    ///
    /// The VTK routines are also designed for SERIAL data, parallel coming...
    ///
    /// This header is templated by the dimension of the data.
    ///
    /// To plot with the plotvtk_* routines:
    ///    plotvtk_begin(...)
    ///    plotvtk_data(...)
    ///    plotvtk_data(...) ...
    ///    plotvtk_end(...)
    ///
    /// NOTE: Paraview expects the structured mesh points in a particular
    /// order, which is why the LowDimIndexIterator is used...
    template<int NDIM>
    void plotvtk_begin(World &world, const char *filename,
        const Vector<double, NDIM> &plotlo, const Vector<double, NDIM> &plothi,
        const Vector<long, NDIM> &npt, bool binary = false) {

        PROFILE_FUNC;
        MADNESS_ASSERT(NDIM>1 && NDIM<=3); // how do we plot data in more than 3-D?

        // our current assumptions
        MADNESS_ASSERT(world.nproc()==1);

        Tensor<double> cell(NDIM, 2);
        int i;
        for(i = 0; i < NDIM; ++i) {
            cell(i, 0) = plotlo[i];
            cell(i, 1) = plothi[i];
        }

        FILE *f=0;
        if(world.rank() == 0) {
            f = fopen(filename, "w");
            if(!f)
                MADNESS_EXCEPTION("plotvtk: failed to open the plot file", 0);

            fprintf(f, "<VTKFile type=\"StructuredGrid\" version=\"0.1\"" \
                " byte_order=\"LittleEndian\" compressor=\"" \
                "vtkZLibDataCompressor\">\n");
            fprintf(f, "  <StructuredGrid WholeExtent=\"");
            for(i = 0; i < NDIM; ++i)
                fprintf(f, "0 %ld ", npt[i]-1);
            for(; i < 3; ++i)
                fprintf(f, "0 0 ");
            fprintf(f, "\">\n");
            fprintf(f, "    <Piece Extent=\"");
            for(i = 0; i < NDIM; ++i)
                fprintf(f, "0 %ld ", npt[i]-1);
            for(; i < 3; ++i)
                fprintf(f, "0 0 ");
            fprintf(f, "\">\n");
            fprintf(f, "      <Points>\n");
            fprintf(f, "        <DataArray NumberOfComponents=\"3\" " \
                "type=\"Float32\" format=\"ascii\">\n");

            Vector<double, NDIM> space;
            for(i = 0; i < NDIM; ++i) {
                if(npt[i] == 1)
                    space[i] = 0.0;
                else
                    space[i] = (cell(i, 1) - cell(i, 0)) / (npt[i] - 1);
            }

            // go through the grid
            for(LowDimIndexIterator it(npt); it; ++it) {
                for(i = 0; i < NDIM; ++i)
                    fprintf(f, "%f ", plotlo[i] + it[i]*space[i]);
                for(; i < 3; ++i)
                    fprintf(f, "0.0 ");
                fprintf(f, "\n");
            }

            fprintf(f, "        </DataArray>\n");
            fprintf(f, "      </Points>\n");
            fprintf(f, "      <PointData>\n");
            fclose(f);
        }
        world.gop.fence();
    }

    /// Generic VTK data writer.
    /// This templated function won't do anything except print a warning
    /// message.  Specialized versions of this function should be used.
    template<typename T, int NDIM>
    void plotvtk_data(const T &function, const char *fieldname, World &world,
        const char *filename, const Vector<double, NDIM> &plotlo,
        const Vector<double, NDIM> &plothi, const Vector<long, NDIM> &npt,
        bool binary = false) {

        MADNESS_EXCEPTION("plotvtk only supports madness::functions", 0);
    }

    /// VTK data writer for real-valued (not complex) madness::functions.
    /// Set plot_refine=true to get a plot of the refinement levels of 
    /// the given function.
    template<typename T, int NDIM>
    void plotvtk_data(const Function<T, NDIM> &function, const char *fieldname,
        World &world, const char *filename, const Vector<double, NDIM> &plotlo,
        const Vector<double, NDIM> &plothi, const Vector<long, NDIM> &npt,
        bool binary = false, bool plot_refine = false) {

        PROFILE_FUNC;
        MADNESS_ASSERT(NDIM>1 && NDIM<=3); // no plotting high-D functions, yet...

        MADNESS_ASSERT(world.nproc()==1);

        Tensor<double> cell(NDIM, 2);
        int i;
        for(i = 0; i < NDIM; ++i) {
            cell(i, 0) = plotlo[i];
            cell(i, 1) = plothi[i];
        }
        std::vector<long> numpt(NDIM);
        for(i = 0; i < NDIM; ++i)
            numpt[i] = npt[i];

        world.gop.barrier();

        function.verify();
        FILE *f = 0;
        if(world.rank() == 0) {
            f = fopen(filename, "a");
            if(!f)
                MADNESS_EXCEPTION("plotvtk: failed to open the plot file", 0);

            fprintf(f, "        <DataArray Name=\"%s\" format=\"ascii\" " \
                "type=\"Float32\" NumberOfComponents=\"1\">\n", fieldname);
        }

        world.gop.fence();
        Tensor<T> tmpr = function.eval_cube(cell, numpt, plot_refine);
        world.gop.fence();

        if(world.rank() == 0) {
            for(LowDimIndexIterator it(numpt); it; ++it) {
                fprintf(f, "%.6e\n", tmpr(*it));
            }
            fprintf(f, "        </DataArray>\n");
            fclose(f);
        }
        world.gop.fence();
    }

    /// VTK data writer for complex-valued madness::functions.
    ///
    /// The complex-value is written as two reals (a vector from VTK's
    /// perspective.  The first (X) component is the real part and the second
    /// (Y) component is the imaginary part.
    /// Set plot_refine=true to get a plot of the refinement levels of 
    /// the given function.
    template<typename T, int NDIM>
    void plotvtk_data(const Function<std::complex<T>, NDIM> &function,
        const char *fieldname, World &world, const char *filename,
        const Vector<double, NDIM> &plotlo, const Vector<double, NDIM> &plothi,
        const Vector<long, NDIM> &npt, bool binary = false, 
        bool plot_refine = false) {

        // this is the same as plotvtk_data for real functions, except the
        // real and imaginary parts are printed on the same line (needed
        // to change NumberOfComponents in the XML tag)

        PROFILE_FUNC;
        MADNESS_ASSERT(NDIM>1 && NDIM<=3); // no plotting high-D functions, yet...

        MADNESS_ASSERT(world.nproc()==1);

        Tensor<double> cell(NDIM, 2);
        int i;
        for(i = 0; i < NDIM; ++i) {
            cell(i, 0) = plotlo[i];
            cell(i, 1) = plothi[i];
        }
        std::vector<long> numpt(NDIM);
        for(i = 0; i < NDIM; ++i)
            numpt[i] = npt[i];

        world.gop.barrier();

        function.verify();
        FILE *f = 0;
        if(world.rank() == 0) {
            f = fopen(filename, "a");
            if(!f)
                MADNESS_EXCEPTION("plotvtk: failed to open the plot file", 0);

            fprintf(f, "        <DataArray Name=\"%s\" format=\"ascii\" " \
                "type=\"Float32\" NumberOfComponents=\"2\">\n", fieldname);
        }

        world.gop.fence();
        Tensor<std::complex<T> > tmpr = function.eval_cube(cell, numpt, 
                                                           plot_refine);
        world.gop.fence();

        if(world.rank() == 0) {
            for(LowDimIndexIterator it(numpt); it; ++it) {
                fprintf(f, "%.6e %.6e\n", real(tmpr(*it)), imag(tmpr(*it)));
            }
            fprintf(f, "        </DataArray>\n");
            fclose(f);
        }
        world.gop.fence();
    }

    /// VTK footer writer.
    template<int NDIM>
    void plotvtk_end(World &world, const char *filename, bool binary = false) {
        PROFILE_FUNC;
        MADNESS_ASSERT(NDIM>1 && NDIM<=3);

        MADNESS_ASSERT(world.nproc() == 1);

        FILE *f = 0;
        if(world.rank() == 0) {
            f = fopen(filename, "a");
            if(!f)
                MADNESS_EXCEPTION("plotvtk: failed to open the plot file", 0);

            fprintf(f, "      </PointData>\n");
            fprintf(f, "      <CellData>\n");
            fprintf(f, "      </CellData>\n");
            fprintf(f, "    </Piece>\n");
            fprintf(f, "  </StructuredGrid>\n");
            fprintf(f, "</VTKFile>\n");
            fclose(f);
        }
        world.gop.fence();
    }


    static inline void plot_line_print_value(FILE* f, double_complex v) {
        fprintf(f, "    %.6e %.6e   ", real(v), imag(v));
    }

    static inline void plot_line_print_value(FILE* f, double v) {
        fprintf(f, " %.6e", v);
    }

    /// Generates ASCII file tabulating f(r) at npoints along line r=lo,...,hi

    /// The ordinate is distance from lo
    template <typename T, int NDIM>
    void plot_line(const char* filename, int npt, const Vector<double,NDIM>& lo, const Vector<double,NDIM>& hi,
                   const Function<T,NDIM>& f) {
        typedef Vector<double,NDIM> coordT;
        coordT h = (hi - lo)*(1.0/(npt-1));

        double sum = 0.0;
        for (int i=0; i<NDIM; i++) sum += h[i]*h[i];
        sum = sqrt(sum);

        World& world = f.world();
        f.reconstruct();
        if (world.rank() == 0) {
            FILE* file = fopen(filename,"w");
            for (int i=0; i<npt; i++) {
                coordT r = lo + h*double(i);
                fprintf(file, "%.6e ", i*sum);
                plot_line_print_value(file, f.eval(r));
                fprintf(file,"\n");
            }
            fclose(file);
        }
        world.gop.fence();
    }

    /// Generates ASCII file tabulating f(r) and g(r) at npoints along line r=lo,...,hi

    /// The ordinate is distance from lo
    template <typename T, typename U, int NDIM>
    void plot_line(const char* filename, int npt, const Vector<double,NDIM>& lo, const Vector<double,NDIM>& hi,
                   const Function<T,NDIM>& f, const Function<U,NDIM>& g) {
        typedef Vector<double,NDIM> coordT;
        coordT h = (hi - lo)*(1.0/(npt-1));

        double sum = 0.0;
        for (int i=0; i<NDIM; i++) sum += h[i]*h[i];
        sum = sqrt(sum);

        World& world = f.world();
        f.reconstruct();
        g.reconstruct();
        if (world.rank() == 0) {
            FILE* file = fopen(filename,"w");
            for (int i=0; i<npt; i++) {
                coordT r = lo + h*double(i);
                fprintf(file, "%.6e ", i*sum);
                plot_line_print_value(file, f.eval(r));
                plot_line_print_value(file, g.eval(r));
                fprintf(file,"\n");
            }
            fclose(file);
        }
        world.gop.fence();
    }


    /// Generates ASCII file tabulating f(r), g(r), and a(r) at npoints along line r=lo,...,hi

    /// The ordinate is distance from lo
    template <typename T, typename U, typename V, int NDIM>
    void plot_line(const char* filename, int npt, const Vector<double,NDIM>& lo, const Vector<double,NDIM>& hi,
                   const Function<T,NDIM>& f, const Function<U,NDIM>& g, const Function<V,NDIM>& a) {
        typedef Vector<double,NDIM> coordT;
        coordT h = (hi - lo)*(1.0/(npt-1));

        double sum = 0.0;
        for (int i=0; i<NDIM; i++) sum += h[i]*h[i];
        sum = sqrt(sum);

        World& world = f.world();
        f.reconstruct();
        g.reconstruct();
        a.reconstruct();
        if (world.rank() == 0) {
            FILE* file = fopen(filename,"w");
            for (int i=0; i<npt; i++) {
                coordT r = lo + h*double(i);
                fprintf(file, "%.6e ", i*sum);
                plot_line_print_value(file, f.eval(r));
                plot_line_print_value(file, g.eval(r));
                plot_line_print_value(file, a.eval(r));
                fprintf(file,"\n");
            }
            fclose(file);
        }
        world.gop.fence();
    }

#endif