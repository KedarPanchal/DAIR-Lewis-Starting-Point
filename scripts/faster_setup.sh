CALLING_DIR="$PWD"
echo "Expecting ${CALLING_DIR} to be the root of the project..."

# Load LMod Modules
module purge
module load \
    GCCcore/13.3.0 \
    GCC/13.3.0 \
    SQLite/3.45.3 \
    GLib/2.80.4 \
    Qt6/6.7.2 \
    MPFR/4.2.1 \
    Boost/1.85.0

# Build CGAL
if [ -d "$SCRATCH/cgal-install" ]; then
    echo "CGAL already built, skipping build..."
else
    echo "Building CGAL..."

    cd $SCRATCH
    if [ -d cgal ]; then
        echo "CGAL already exists, skipping clone..."
    else
        echo "Cloning CGAL..."
        git clone https://github.com/CGAL/cgal.git --depth 1
    fi
    cd cgal
    echo "Building CGAL..."
    cmake -S . -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=$SCRATCH/cgal-install \
        -DWITH_CGAL_Qt6=ON
    cmake --install build
fi

# Build the project
cd $CALLING_DIR
cmake -S . -B build -DCGAL_PREFIX="$SCRATCH/cgal-install"
cmake --build build
