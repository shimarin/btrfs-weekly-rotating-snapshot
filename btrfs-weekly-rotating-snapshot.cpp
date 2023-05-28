/*
 * btrfs-weekly-rotating-snapshot.cpp
 * Copyright 2023 Tomoatsu Shimada
 */
#include <unistd.h>
#include <iostream>
#include <filesystem>

#include <btrfsutil.h>
#include <argparse/argparse.hpp>

/**
 * @brief Take snapshot of specified btrfs mountpoint
 * @param path Path to the mountpoint
 * @return Path to the snapshot
 */
static std::filesystem::path snapshot(const std::filesystem::path& path)
{
    if (btrfs_util_is_subvolume(path.c_str()) != BTRFS_UTIL_OK) {
        throw std::runtime_error(path.string() + " is offline or not a btrfs volume");
    }
    //else
    auto head = path / ".snapshots/head";

    // create snapshot
    if (btrfs_util_is_subvolume(head.c_str()) == BTRFS_UTIL_OK) {
        struct btrfs_util_subvolume_info subvol;
        auto rst = btrfs_util_subvolume_info(head.c_str(), 0, &subvol);
        if (rst != BTRFS_UTIL_OK) {
            throw std::runtime_error("Inspecting subvolume " + head.string() + " failed(" + btrfs_util_strerror(rst) + ")");
        }
        //else

        static const char* DOWSTR[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        auto dow = path / ".snapshots" / DOWSTR[localtime(&subvol.otime.tv_sec)->tm_wday];
        if (btrfs_util_is_subvolume(dow.c_str()) == BTRFS_UTIL_OK) {
            btrfs_util_delete_subvolume(dow.c_str(), BTRFS_UTIL_DELETE_SUBVOLUME_RECURSIVE);
            std::cerr << "Snapshot " << dow << " deleted" << std::endl;
        }
        std::filesystem::rename(head, dow);
        std::cerr << "Snapshot " << head << " renamed to " << dow << std::endl;
    }
    std::filesystem::create_directory(path / ".snapshots");
    auto rst = btrfs_util_create_snapshot(path.c_str(), head.c_str(), BTRFS_UTIL_CREATE_SNAPSHOT_READ_ONLY, NULL, NULL);
    if (rst != BTRFS_UTIL_OK) {
        throw std::runtime_error("Creating readonly snapshot " + head.string() + " failed(" + btrfs_util_strerror(rst) + ")");
    }
    sync();
    return head;
}

int main(int argc, char* argv[])
{
    argparse::ArgumentParser program("btrfs-weekly-rotating-snapshot");
    program.add_argument("path").help("Path to the btrfs mountpoint");
    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    try {
        auto path = program.get<std::string>("path");
        auto rst = snapshot(path);
        std::cout << "Snapshot " << rst << " created." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}