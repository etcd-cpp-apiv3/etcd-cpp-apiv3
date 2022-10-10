vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO etcd-cpp-apiv3/etcd-cpp-apiv3
    REF 2e38d3c11e02366906328d5905ac934311761cd0
    SHA512 ca6613b821643050701cbfcb7f9f35c48f0ea9a9d86b73b0401cbacf66a50a1c98b14e07fe8f0939dba379ceb2cf6c60093158b1b4a20c731984c47875b97f90
    HEAD_REF master
)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
    OPTIONS
        -DBUILD_ETCD_TESTS=OFF
)
set(VCPKG_POLICY_DLLS_WITHOUT_LIBS enabled)
set(VCPKG_POLICY_DLLS_WITHOUT_EXPORTS enabled)

vcpkg_install_cmake()
vcpkg_fixup_cmake_targets(CONFIG_PATH lib/cmake/etcd-cpp-api)

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/share)

vcpkg_copy_pdbs()

# Handle copyright
file(INSTALL ${SOURCE_PATH}/LICENSE.txt DESTINATION ${CURRENT_PACKAGES_DIR}/share/etcd-cpp-apiv3 RENAME copyright)
