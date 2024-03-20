project('epics-pv-database', 'cpp', version: '4.6.1', license: 'EPICS')

epics_com_dep = dependency('epics-com')
epics_pv_data_dep = dependency('epics-pv-data')
epics_pv_access_dep = dependency('epics-pv-access')

headers = [
  'src/pv/channelProviderLocal.h',
  'src/pv/controlSupport.h',
  'src/pv/pvArrayPlugin.h',
  'src/pv/pvDatabase.h',
  'src/pv/pvDeadbandPlugin.h',
  'src/pv/pvPlugin.h',
  'src/pv/pvStructureCopy.h',
  'src/pv/pvSupport.h',
  'src/pv/pvTimestampPlugin.h',
  'src/pv/pvdbcrAddRecord.h',
  'src/pv/pvdbcrProcessRecord.h',
  'src/pv/pvdbcrRemoveRecord.h',
  'src/pv/pvdbcrScalarArrayRecord.h',
  'src/pv/pvdbcrScalarRecord.h',
  'src/pv/pvdbcrTraceRecord.h',
  'src/pv/scalarAlarmSupport.h',
]

sources = [
  'src/copy/pvArrayPlugin.cpp',
  'src/copy/pvCopy.cpp',
  'src/copy/pvDeadbandPlugin.cpp',
  'src/copy/pvPlugin.cpp',
  'src/copy/pvTimestampPlugin.cpp',

  'src/database/pvDatabase.cpp',
  'src/database/pvRecord.cpp',

  'src/pvAccess/channelLocal.cpp',
  'src/pvAccess/channelProviderLocal.cpp',
  'src/pvAccess/monitorFactory.cpp',
  'src/pvAccess/registerChannelProviderLocal.cpp',

  'src/special/pvdbcrAddRecord.cpp',
  'src/special/pvdbcrProcessRecord.cpp',
  'src/special/pvdbcrRemoveRecord.cpp',
  'src/special/pvdbcrScalarArrayRecord.cpp',
  'src/special/pvdbcrScalarRecord.cpp',
  'src/special/pvdbcrTraceRecord.cpp',

  'src/support/controlSupport.cpp',
  'src/support/scalarAlarmSupport.cpp',
]

include_directories = ['src']

libpv_database = both_libraries(
  'epics-pv-database',
  sources,
  include_directories: include_directories,
  dependencies: [epics_com_dep, epics_pv_data_dep, epics_pv_access_dep],
  install: true,
)

libpv_database_dep = declare_dependency(
  include_directories: include_directories,
  link_with: libpv_database,
)

pkgconfig = import('pkgconfig')
pkgconfig.generate(libpv_database)

install_headers(headers, subdir: 'pv')
