CALLING_DIR="$PWD"
echo "Expecting ${CALLING_DIR} to be the root of the project..."

# Load LMod Modules
# TODO: Figure out the version of MPFR and Boost to load
module load \
    GCCcore/13.3.0 \
    SQLite/3.45.3 \
    Qt6/6.7.2 \
    MPFR/4.2.1 \
    Boost/1.85.0

# Build CGAL
cd $SCRATCH
git clone https://github.com/CGAL/cgal.git --depth 1
cd cgal
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$SCRATCH/cgal-install \
    -DWITH_CGAL_Qt6=ON
cmake --install build

# Build the project
cd $CALLING_DIR
cmake -S . -B build -DCGAL_PREFIX="$SCRATCH/cgal-install"
cmake --build build
