#! /bin/ksh
#-----------------------------------------------------------------------------
# This is just an example script to install the hide program, you must be
# logged in as the Oracle account and ORACLE_HOME must be set.
#-----------------------------------------------------------------------------
#
# USAGE: hide_install.sh [install|uninstall] [program_1 program_2 program_n]
#
#-----------------------------------------------------------------------------
DefFileList="sqlplus sqlload sqlldr exp imp"

typeset -l Action
Action=${1}
 
if [ "${Action}" != "install" -a "${Action}" != "uninstall" ]
then
	print "ERROR: Action not supported: \"${Action}\"!" >&2
	print "USAGE: ${0} [install|uninstall] [program_1 program_2 program_n]" >&2
	exit 1
fi

shift 1
FileList=${*}
if [ -z "${FileList}" ]
then
	FileList=${DefFileList}
fi

if [ -z "${ORACLE_HOME}" ]
then
	print "ERROR: ORACLE_HOME must be set!" >&2
	exit 1
fi

if [ ! -d ${ORACLE_HOME} ]
then
	print "ERROR: ORACLE_HOME not found: \"${ORACLE_HOME}\"!" >&2
	exit 1
fi

# Move to the bin directory.
cd ${ORACLE_HOME}/bin

HideCmd=hide

if [ ! -x ${HideCmd} ]
then
	print "ERROR: \"${HideCmd}\" not found in \"${ORACLE_HOME}/bin\"!" >&2
	exit 1
fi

for TargetFile in ${FileList}
do
	if [ ${Action} = "install" ]
	then
		if [ ! -f ${TargetFile} ]
		then
			print "ERROR: target file not found: \"${TargetFile}\" skipping!" >&2
		elif [ -f ${TargetFile}.hide ]
		then
			print "WARNING: hide already installed for: \"${TargetFile}\"!" >&2
		else
			print "Installing hide for: \"${TargetFile}\"!" >&2
			mv ${TargetFile} ${TargetFile}.hide
			ln -s ${HideCmd} ${TargetFile}
		fi
	elif [ ${Action} = "uninstall" ]
	then
		if [ ! -f ${TargetFile} ]
		then
			print "ERROR: target file not found: \"${TargetFile}\" skipping!" >&2
		elif [ -f ${TargetFile}.hide ]
		then
			print "Uninstalling hide for: \"${TargetFile}\"!" >&2
			if [ -L ${TargetFile} ]
			then
				rm ${TargetFile}
			else
				mv ${TargetFile} ${TargetFile}.`date +%m%d%H%M`
			fi
			mv ${TargetFile}.hide ${TargetFile}
		else
			print "WARNING: hide not installed for: \"${TargetFile}\"!" >&2
		fi
	fi
done
