/*
 * QDigiDocCrypto
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "LdapSearch.h"

#include <QtCore/QTimerEvent>
#include <QtNetwork/QSslCertificate>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <Winldap.h>
#include <Winber.h>
#else
#define LDAP_DEPRECATED 1
#include <sys/time.h>
#include <ldap.h>
#define ULONG int
#define LDAP_TIMEVAL timeval
#endif


class LdapSearchPrivate
{
public:
	LdapSearchPrivate(): ldap(0), msg_id(0) {}

	LDAP *ldap;
	ULONG msg_id;
};

LdapSearch::LdapSearch( QObject *parent )
:	QObject( parent )
,	d( new LdapSearchPrivate )
{}

LdapSearch::~LdapSearch()
{
	if( d->ldap )
		ldap_unbind_s( d->ldap );
	delete d;
}

bool LdapSearch::init()
{
	if( d->ldap )
		return true;

	if( !(d->ldap = ldap_init( "ldap.sk.ee", 389 )) )
	{
		setLastError( tr("Failed to init ldap"), -1 );
		return false;
	}

	int version = LDAP_VERSION3;
	ldap_set_option( d->ldap, LDAP_OPT_PROTOCOL_VERSION, &version );

	int err = ldap_simple_bind_s( d->ldap, 0, 0 );
	if( err )
		setLastError( tr("Failed to init ldap"), err );
	return !err;
}

void LdapSearch::search( const QString &search )
{
	if( !init() )
		return;

	char *attrs[] = { const_cast<char*>("userCertificate;binary"), 0 };

	int err = ldap_search_ext( d->ldap, "c=EE", LDAP_SCOPE_SUBTREE,
		const_cast<char*>(search.toLocal8Bit().constData()), attrs, 0, 0, 0, 0, 0, &d->msg_id );
	if( err )
		setLastError( tr("Failed to init ldap search"), err );
	else
		startTimer( 1000 );
}

void LdapSearch::setLastError( const QString &msg, int err )
{
	QString res = msg;
	QString details;
	switch( err )
	{
	case -1:
		break;
	case LDAP_UNAVAILABLE:
		res += "<br />";
		res += tr("LDAP server is unavailable.");
		break;
	default:
		details = tr( "Error Code: %1 (%2)" ).arg( err ).arg( ldap_err2string( err ) );
		break;
	}
	Q_EMIT error( res, details );
}

void LdapSearch::timerEvent( QTimerEvent *e )
{
	LDAPMessage *result = 0;
	LDAP_TIMEVAL t = { 5, 0 };
	int err = ldap_result( d->ldap, d->msg_id, LDAP_MSG_ALL, &t, &result );
	switch( err )
	{
	case LDAP_SUCCESS: //Timeout
		return;
	case LDAP_RES_SEARCH_ENTRY:
	case LDAP_RES_SEARCH_RESULT:
		break;
	default:
		setLastError( tr("Failed to get result"), err );
		killTimer( e->timerId() );
		return;
	}
	killTimer( e->timerId() );

	QList<QSslCertificate> list;
	for( LDAPMessage *entry = ldap_first_entry( d->ldap, result );
		 entry; entry = ldap_next_entry( d->ldap, entry ) )
	{
		BerElement *pos = 0;
		for( char *attr = ldap_first_attribute( d->ldap, entry, &pos );
			 attr; attr = ldap_next_attribute( d->ldap, entry, pos ) )
		{
			if( qstrcmp( attr, "userCertificate;binary" ) == 0 )
			{
				berval **cert = ldap_get_values_len( d->ldap, entry, attr );
				for( ULONG i = 0; i < ldap_count_values_len( cert ); ++i )
					list << QSslCertificate( QByteArray( cert[i]->bv_val, cert[i]->bv_len ), QSsl::Der );
				ldap_value_free_len( cert );
			}
			ldap_memfree( attr );
		}
		ber_free( pos, 0 );
	}
	ldap_msgfree( result );

	Q_EMIT searchResult( list );
}
