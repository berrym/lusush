Name: lusush
Version: v0.1.0
Release: 1%{?dist}
Summary: A tiny shell program
License: BSD2CLAUSE

URL: https://github.com/berrym/lusush
Source0: %{URL}/archive/%{version}.tar.gz

BuildRequires: meson
BuildRequires: gcc
BuildRequires: pkgconfig(readline)

BuildArch: x86_64

%description
A tiny *NIX like shell.

%global debug_package %{nil}

%prep
%autosetup

%build
%meson
%meson_build
ninja -C build

%install
%meson_install

%check
%meson_test

%files
%doc README

%changelog
* Mon Aug 17 2020 Michael Berry <trismegustis@gmail.com> - 
- 

