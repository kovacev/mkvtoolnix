dnl
dnl Check for static
dnl

AC_ARG_ENABLE([static], AC_HELP_STRING([--enable-static],[make a static build of the applications (no)]), [], [enable_static=no])

STATIC_LIBS=""

if test x"$enable_static" = xyes ; then
  STATIC_LIBS="  -lpthread -static  "
  opt_features_yes="$opt_features_yes\n   * make a static build of the applications"
else
  opt_features_no="$opt_features_no\n   * make a static build of the applications"
fi

AC_SUBST(STATIC_LIBS)
