if [ -d extra ]; then
    rm -rf extra
fi

dolce-libs-gen -c extra.yml extra
cd extra
cmake .
make
cd ..


if [ -d build ]; then
    rm -rf build
    mkdir build
fi

cd build
cmake ../
make
cd ..