searchd_args = [
  '-DG_LOG_USE_STRUCTURED',
  '-DG_LOG_DOMAIN="sm.puri.Phosh.Search"'
]

executable('phosh-searchd', ['searchd.c', 'search-provider.c'],
           dependencies: phoshsearch_dep,
           c_args: searchd_args,
           install_path: libexecdir,
           install: true)
