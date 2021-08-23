vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO etcd-cpp-apiv3/etcd-cpp-apiv3
    REF 1b68f9c79dede1264a0977a239e497358760b0ac
    SHA512 b575cbd76a50ae2869c234050aadfe5201a580554ff617043b6d0eac4b08331b5cd8e6fb7c218d74ae756d8971a68f761c665e3c9e868b5d447c3f0c9311abc5
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
