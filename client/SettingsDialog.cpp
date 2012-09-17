/*
 * QDigiDocClient
 *
 * Copyright (C) 2009-2012 Jargo Kõster <jargo@innovaatik.ee>
 * Copyright (C) 2009-2012 Raul Metsma <raul@innovaatik.ee>
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

#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"

#include "AccessCert.h"
#include "Application.h"

#include <common/CertificateWidget.h>
#include <common/FileDialog.h>
#include <common/Settings.h>
#include <common/SslCertificate.h>

#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>
#include <QtGui/QDropEvent>
#include <QtGui/QMessageBox>

Q_DECLARE_METATYPE(QSslCertificate)

SettingsDialog::SettingsDialog( QWidget *parent )
:	QWidget( parent )
,	d( new Ui::SettingsDialog )
{
	d->setupUi( this );
	setAttribute( Qt::WA_DeleteOnClose );
	setWindowFlags( Qt::Sheet );
	d->p12Cert->installEventFilter( this );
	Common::setAccessibleName( d->p12Label );

	Settings s;
	s.beginGroup( "Client" );

	d->defaultSameDir->setChecked( s.value( "DefaultDir" ).isNull() );
	d->defaultDir->setText( s.value( "DefaultDir" ).toString() );
	d->showIntro->setChecked( s.value( "Intro", true ).toBool() );
#ifdef APPSTORE
	d->askSaveAs->hide();
	QSslCertificate c = AccessCert::cert();
	d->showP12Cert->setEnabled( !c.isNull() );
	d->showP12Cert->setProperty( "cert", QVariant::fromValue( c ) );
#else
	d->askSaveAs->setChecked( s.value( "AskSaveAs", true ).toBool() );
	d->p12Cert->setText( Application::confValue( Application::PKCS12Cert ).toString() );
	d->p12Pass->setText( Application::confValue( Application::PKCS12Pass ).toString() );
#endif

	const QString type = s.value( "type", "ddoc" ).toString();
	d->typeBDoc->setChecked( type == "bdoc" );
	d->typeDDoc->setChecked( type == "ddoc" );

	d->signRoleInput->setText( s.value( "Role" ).toString() );
	d->signResolutionInput->setText( s.value( "Resolution" ).toString() );
	d->signCityInput->setText( s.value( "City" ).toString() );
	d->signStateInput->setText( s.value( "State" ).toString() );
	d->signCountryInput->setText( s.value( "Country" ).toString() );
	d->signZipInput->setText( s.value( "Zip" ).toString() );

	d->signOverwrite->setChecked( s.value( "Overwrite", false ).toBool() );

	d->proxyHost->setText( Application::confValue( Application::ProxyHost ).toString() );
	d->proxyPort->setText( Application::confValue( Application::ProxyPort ).toString() );
	d->proxyUser->setText( Application::confValue( Application::ProxyUser ).toString() );
	d->proxyPass->setText( Application::confValue( Application::ProxyPass ).toString() );
	d->p12Ignore->setChecked( Application::confValue( Application::PKCS12Disable, false ).toBool() );

	s.endGroup();
}

SettingsDialog::~SettingsDialog() { delete d; }

bool SettingsDialog::eventFilter( QObject *o, QEvent *e )
{
	if( o == d->p12Cert && e->type() == QEvent::Drop )
	{
		QDropEvent *d = static_cast<QDropEvent*>(e);
		if( d->mimeData()->hasUrls() )
		{
			setP12Cert( d->mimeData()->urls().value( 0 ).toLocalFile() );
			return true;
		}
	}
	return QWidget::eventFilter( o, e );
}

void SettingsDialog::on_p12Button_clicked()
{
	QString cert = Application::confValue( Application::PKCS12Cert ).toString();
	if( cert.isEmpty() )
		cert = QDesktopServices::storageLocation( QDesktopServices::DocumentsLocation );
	else
		cert = QFileInfo( cert ).path();
	cert = FileDialog::getOpenFileName( this, tr("Select server access certificate"), cert,
		tr("Server access certificates (*.p12 *.p12d *.pfx)") );
	if( !cert.isEmpty() )
		setP12Cert( cert );
}

void SettingsDialog::on_p12Install_clicked()
{
#ifndef APPSTORE
	d->showP12Cert->setEnabled( false );
	d->p12Error->clear();
	PKCS12Certificate p12 = PKCS12Certificate::fromPath(
		d->p12Cert->text(), d->p12Pass->text() );
	switch( p12.error() )
	{
	case PKCS12Certificate::FailedToRead:
	case PKCS12Certificate::FileNotExist:
	case PKCS12Certificate::NullError:
		d->showP12Cert->setEnabled( !p12.isNull() );
		d->showP12Cert->setProperty( "cert", QVariant::fromValue( p12.certificate() ) );
		break;
	case PKCS12Certificate::InvalidPasswordError:
		d->p12Error->setText( tr("Invalid password") );
		break;
	default:
		d->p12Error->setText( tr("Server access certificate error: %1").arg( p12.errorString() ) );
		break;
	}
#else
	QFile f( d->p12Cert->text() );
	f.open( QFile::ReadOnly );
	AccessCert().installCert( f.readAll(), d->p12Pass->text() );
	QSslCertificate c = AccessCert::cert();
	d->showP12Cert->setEnabled( !c.isNull() );
	d->showP12Cert->setProperty( "cert", QVariant::fromValue( c ) );
#endif
}

void SettingsDialog::on_selectDefaultDir_clicked()
{
	QString dir = Settings().value( "Client/DefaultDir" ).toString();
	dir = FileDialog::getExistingDirectory( this, tr("Select folder"), dir );
	if( !dir.isEmpty() )
	{
		Settings().setValue( "Client/DefaultDir", dir );
		d->defaultDir->setText( dir );
	}
	d->defaultSameDir->setChecked( d->defaultDir->text().isEmpty() );
}

void SettingsDialog::on_showP12Cert_clicked()
{
	CertificateDialog(
		d->showP12Cert->property( "cert" ).value<QSslCertificate>() ).exec();
}

void SettingsDialog::on_typeBDoc_clicked( bool checked )
{
	if( !checked )
		return;

	QMessageBox b( QMessageBox::Information, windowTitle(), tr(
		"We currently do not recommend use of BDOC format as it will be changed in "
		"near future to comply with new international standards. Please use DDOC "
		"format instead. More information on BDOC format is available at "
		"<a href=\"http://www.id.ee/eng/bdoc\">http://www.id.ee/eng/bdoc</a>" ),
		QMessageBox::NoButton, this );
	if( QLabel *l = b.findChild<QLabel*>() )
	{
		l->setTextInteractionFlags( Qt::LinksAccessibleByKeyboard|Qt::LinksAccessibleByMouse );
		Common::setAccessibleName( l );
	}
	b.exec();
}

void SettingsDialog::save()
{
	Settings s;
	s.beginGroup( "Client" );
	s.setValue( "Intro", d->showIntro->isChecked() );
	s.setValue( "Overwrite", d->signOverwrite->isChecked() );
#ifndef APPSTORE
	s.setValue( "AskSaveAs", d->askSaveAs->isChecked() );
	Application::setConfValue( Application::PKCS12Cert, d->p12Cert->text() );
	Application::setConfValue( Application::PKCS12Pass, d->p12Pass->text() );
#endif
	s.setValue( "type", d->typeBDoc->isChecked() ? "bdoc" : "ddoc" );
	if( d->defaultSameDir->isChecked() )
	{
		d->defaultDir->clear();
		s.remove( "DefaultDir" );
	}

	Application::setConfValue( Application::ProxyHost, d->proxyHost->text() );
	Application::setConfValue( Application::ProxyPort, d->proxyPort->text() );
	Application::setConfValue( Application::ProxyUser, d->proxyUser->text() );
	Application::setConfValue( Application::ProxyPass, d->proxyPass->text() );
	Application::setConfValue( Application::PKCS12Disable, d->p12Ignore->isChecked() );

	saveSignatureInfo(
		d->signRoleInput->text(),
		d->signResolutionInput->text(),
		d->signCityInput->text(),
		d->signStateInput->text(),
		d->signCountryInput->text(),
		d->signZipInput->text(),
		true );
}

void SettingsDialog::saveSignatureInfo(
		const QString &role,
		const QString &resolution,
		const QString &city,
		const QString &state,
		const QString &country,
		const QString &zip,
		bool force )
{
	Settings s;
	s.beginGroup( "Client" );
	if( force || s.value( "Overwrite", "false" ).toBool() )
	{
		s.setValue( "Role", role );
		s.setValue( "Resolution", resolution );
		s.setValue( "City", city );
		s.setValue( "State", state ),
		s.setValue( "Country", country );
		s.setValue( "Zip", zip );
	}
	s.endGroup();
}

void SettingsDialog::setP12Cert( const QString &cert )
{
	d->p12Cert->setText( cert );
	d->tabWidget->setCurrentIndex( 1 );
}

void SettingsDialog::setPage( int page ) { d->tabWidget->setCurrentIndex( page ); }
