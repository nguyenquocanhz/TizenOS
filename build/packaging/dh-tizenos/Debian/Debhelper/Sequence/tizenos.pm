#!/usr/bin/perl
# Debian/Debhelper/Sequence/tizenos.pm
use warnings;
use strict;
use Debian::Debhelper::Dh_Lib;

# Thêm các bước riêng của TizenOS vào chu trình debhelper
# insert_after hoặc insert_before

insert_after("dh_systemd_enable", "dh_tizenos_systemd");
insert_after("dh_install", "dh_tizenos_dbus");
insert_after("dh_fixperms", "dh_tizenos_permissions");

1;
