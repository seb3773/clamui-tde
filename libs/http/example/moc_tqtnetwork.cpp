/****************************************************************************
** TQtNetworkReply meta object code from reading C++ file 'tqtnetwork.h'
**
** Created by: The TQt Meta Object Compiler version 26 (TQt 3.5.0)
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef TQT_NO_COMPAT
#include "../tqtnetwork.h"
#include <ntqmetaobject.h>
#include <ntqapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 26)
#error "This file was generated using the moc from 3.5.0. It"
#error "cannot be used with the include files from this version of TQt."
#error "(The moc has changed too much.)"
#endif

const char *TQtNetworkReply::className() const
{
    return "TQtNetworkReply";
}

TQMetaObject *TQtNetworkReply::metaObj = 0;
static TQMetaObjectCleanUp cleanUp_TQtNetworkReply( "TQtNetworkReply", &TQtNetworkReply::staticMetaObject );

#ifndef TQT_NO_TRANSLATION
TQString TQtNetworkReply::tr( const char *s, const char *c )
{
    if ( tqApp )
	return tqApp->translate( "TQtNetworkReply", s, c, TQApplication::DefaultCodec );
    else
	return TQString::fromLatin1( s );
}
#ifndef TQT_NO_TRANSLATION_UTF8
TQString TQtNetworkReply::trUtf8( const char *s, const char *c )
{
    if ( tqApp )
	return tqApp->translate( "TQtNetworkReply", s, c, TQApplication::UnicodeUTF8 );
    else
	return TQString::fromUtf8( s );
}
#endif // TQT_NO_TRANSLATION_UTF8

#endif // TQT_NO_TRANSLATION

TQMetaObject* TQtNetworkReply::staticMetaObject()
{
    if ( metaObj ) {
	return metaObj;
}
#ifdef TQT_THREAD_SUPPORT
    if (tqt_sharedMetaObjectMutex) tqt_sharedMetaObjectMutex->lock();
    if ( metaObj ) {
	if (tqt_sharedMetaObjectMutex) tqt_sharedMetaObjectMutex->unlock();
	return metaObj;
    }
#endif // TQT_THREAD_SUPPORT
    TQMetaObject* parentObject = TQObject::staticMetaObject();
    static const TQUMethod signal_0 = {"finished", 0, 0 };
    static const TQUParameter param_signal_1[] = {
	{ "message", &static_QUType_TQString, 0, TQUParameter::In }
    };
    static const TQUMethod signal_1 = {"error", 1, param_signal_1 };
    static const TQMetaData signal_tbl[] = {
	{ "finished()", &signal_0, TQMetaData::Public },
	{ "error(const TQString&)", &signal_1, TQMetaData::Public }
    };
    metaObj = TQMetaObject::new_metaobject(
	"TQtNetworkReply", parentObject,
	0, 0,
	signal_tbl, 2,
#ifndef TQT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // TQT_NO_PROPERTIES
	0, 0 );
    cleanUp_TQtNetworkReply.setMetaObject( metaObj );
#ifdef TQT_THREAD_SUPPORT
    if (tqt_sharedMetaObjectMutex) tqt_sharedMetaObjectMutex->unlock();
#endif // TQT_THREAD_SUPPORT
    return metaObj;
}

void* TQtNetworkReply::tqt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "TQtNetworkReply" ) )
	return this;
    return TQObject::tqt_cast( clname );
}

// SIGNAL finished
void TQtNetworkReply::finished()
{
    activate_signal( staticMetaObject()->signalOffset() + 0 );
}

// SIGNAL error
void TQtNetworkReply::error( const TQString& t0 )
{
    activate_signal( staticMetaObject()->signalOffset() + 1, t0 );
}

bool TQtNetworkReply::tqt_invoke( int _id, TQUObject* _o )
{
    return TQObject::tqt_invoke(_id,_o);
}

bool TQtNetworkReply::tqt_emit( int _id, TQUObject* _o )
{
    switch ( _id - staticMetaObject()->signalOffset() ) {
    case 0: finished(); break;
    case 1: error((const TQString&)static_QUType_TQString.get(_o+1)); break;
    default:
	return TQObject::tqt_emit(_id,_o);
    }
    return TRUE;
}
#ifndef TQT_NO_PROPERTIES

bool TQtNetworkReply::tqt_property( int id, int f, TQVariant* v)
{
    return TQObject::tqt_property( id, f, v);
}

bool TQtNetworkReply::tqt_static_property( TQObject* , int , int , TQVariant* ){ return FALSE; }
#endif // TQT_NO_PROPERTIES


const char *TQtNetworkAccessManager::className() const
{
    return "TQtNetworkAccessManager";
}

TQMetaObject *TQtNetworkAccessManager::metaObj = 0;
static TQMetaObjectCleanUp cleanUp_TQtNetworkAccessManager( "TQtNetworkAccessManager", &TQtNetworkAccessManager::staticMetaObject );

#ifndef TQT_NO_TRANSLATION
TQString TQtNetworkAccessManager::tr( const char *s, const char *c )
{
    if ( tqApp )
	return tqApp->translate( "TQtNetworkAccessManager", s, c, TQApplication::DefaultCodec );
    else
	return TQString::fromLatin1( s );
}
#ifndef TQT_NO_TRANSLATION_UTF8
TQString TQtNetworkAccessManager::trUtf8( const char *s, const char *c )
{
    if ( tqApp )
	return tqApp->translate( "TQtNetworkAccessManager", s, c, TQApplication::UnicodeUTF8 );
    else
	return TQString::fromUtf8( s );
}
#endif // TQT_NO_TRANSLATION_UTF8

#endif // TQT_NO_TRANSLATION

TQMetaObject* TQtNetworkAccessManager::staticMetaObject()
{
    if ( metaObj ) {
	return metaObj;
}
#ifdef TQT_THREAD_SUPPORT
    if (tqt_sharedMetaObjectMutex) tqt_sharedMetaObjectMutex->lock();
    if ( metaObj ) {
	if (tqt_sharedMetaObjectMutex) tqt_sharedMetaObjectMutex->unlock();
	return metaObj;
    }
#endif // TQT_THREAD_SUPPORT
    TQMetaObject* parentObject = TQObject::staticMetaObject();
    metaObj = TQMetaObject::new_metaobject(
	"TQtNetworkAccessManager", parentObject,
	0, 0,
	0, 0,
#ifndef TQT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // TQT_NO_PROPERTIES
	0, 0 );
    cleanUp_TQtNetworkAccessManager.setMetaObject( metaObj );
#ifdef TQT_THREAD_SUPPORT
    if (tqt_sharedMetaObjectMutex) tqt_sharedMetaObjectMutex->unlock();
#endif // TQT_THREAD_SUPPORT
    return metaObj;
}

void* TQtNetworkAccessManager::tqt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "TQtNetworkAccessManager" ) )
	return this;
    return TQObject::tqt_cast( clname );
}

bool TQtNetworkAccessManager::tqt_invoke( int _id, TQUObject* _o )
{
    return TQObject::tqt_invoke(_id,_o);
}

bool TQtNetworkAccessManager::tqt_emit( int _id, TQUObject* _o )
{
    return TQObject::tqt_emit(_id,_o);
}
#ifndef TQT_NO_PROPERTIES

bool TQtNetworkAccessManager::tqt_property( int id, int f, TQVariant* v)
{
    return TQObject::tqt_property( id, f, v);
}

bool TQtNetworkAccessManager::tqt_static_property( TQObject* , int , int , TQVariant* ){ return FALSE; }
#endif // TQT_NO_PROPERTIES
