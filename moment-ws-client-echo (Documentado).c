#include <libwebsockets.h>
#include <string.h>
#include <signal.h>

#define LWS_PLUGIN_STATIC
#include "moment_protocol_lws_client_echo.c"

static struct lws_protocols protocolos[] = { /*Arreglo de estructuras del protocolo de lws*/
	LWS_PLUGIN_PROTOCOL_MINIMAL_CLIENT_ECHO,
	{ NULL, NULL, 0, 0 } /* terminacion */
};

static struct lws_context *context;
static int interrupted, port = 7681, options = 0;
static const char *url = "/", *ads = "localhost", *iface = NULL;

 /*El protocolo muestra como enviar y recibir archivos a través de un servidor Websocket*/

/*Pvo (per virtual host option), Permite que un servidor comparta sus recursos, 
como los ciclos de memoria y procesador, sin requerir que todos los servicios
 usen el mismo nombre de host.*/






static const struct lws_protocol_vhost_options pvo_iface = {

	/* lws_protocol_vhost_options pvo_iface, es una lista ligada (nombre de protocolo-valor)*/ 
	
	NULL,
	NULL,
	"iface",		/* nombre */
	(void *)&iface	/* valor */
};

static const struct lws_protocol_vhost_options pvo_ads = {
	&pvo_iface,
	NULL,
	"ads",		/* nombre */
	(void *)&ads	/* valor */
};

static const struct lws_protocol_vhost_options pvo_url = {
	&pvo_ads,
	NULL,
	"url",		/* nombre */	(void *)&url	/* valor */
};

static const struct lws_protocol_vhost_options pvo_options = {
	&pvo_url,
	NULL,
	"options",		/* nombre */
	(void *)&options	/* valor */
};

static const struct lws_protocol_vhost_options pvo_port = {
	&pvo_options,
	NULL,
	"port",		/* nombre */
	(void *)&port	/* valor */
};

static const struct lws_protocol_vhost_options pvo_interrupted = {
	&pvo_port,
	NULL,
	"interrupted",		/* nombre */
	(void *)&interrupted	/* valor */
};

static const struct lws_protocol_vhost_options pvo = {
	NULL,		/* "next" pvo linked-list */
	&pvo_interrupted,	/* "child" pvo linked-list */
	"lws-minimal-client-echo",	/* protocol name we belong to on this vhost */
	""		/* ignored */
};
static const struct lws_extension extensions[] = {
	{
		"permessage-deflate",
		lws_extension_callback_pm_deflate,
		"permessage-deflate"
		 "; client_no_context_takeover"
		 "; client_max_window_bits"
	},
	{ NULL, NULL, NULL /* terminator */ }
};

void sigint_handler(int sig)
{
	interrupted = 1;
}

int main(int argc, const char **argv)
{

    /* Declaración de variables necesarios para el programa */
    /* lws_context_creation_info:  Estructura de información necesaria para que la libreria
                                   libwebsockets necesita para establecer la conexión a nuestro
                                   servidor momentos esto lo almacenamos en la variable info */
	struct lws_context_creation_info info;

	/* Declaración de variables que almacenan el nivel de logs para la salida estandar en la
	   libreria: 
	   LLL_USER: Logear datos de usuario
	   LLL_ERR:  Logear de errores
	   LLL_WARN: Logear warnings  
	   LLL_NOTICE: Logear avisos 

	   Al pasar en la linea de comando el parámetro -d se puede indicar es opcional */
	const char *p;
	int n, logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE

	if ((p = lws_cmdline_option(argc, argv, "-d")))
		logs = atoi(p);

    /* Establecemos el nivel de log e imprimimos como utilizar el programa */
	lws_set_log_level(logs, NULL);
	lwsl_user("Cliente Momentos receptor de la notificación de algún momento disponible\n");
	lwsl_user("   lws-minimal-ws-client-echo [-n (no exts)] [-u url] [-p port] [-o (once)]\n");

    /* Leer los PARAMETROS DE ENTRADA PROPORCIONADOS EN LA LINEA DE COMANDOS */
	if ((p = lws_cmdline_option(argc, argv, "-u")))
		url = p; //Dirección URL del servidor websockets momentos: se proporciona /

	if ((p = lws_cmdline_option(argc, argv, "-p")))
		port = atoi(p); //Puerto donde escucha el servidor websockets momentos: 8887

	if (lws_cmdline_option(argc, argv, "-o"))
		options |= 1; //Opciones de conexión - opcional

	if (lws_cmdline_option(argc, argv, "--ssl"))
		options |= 2; //Opciones de conexión segura SSL (NO LA UTILIZAMOS)

	if ((p = lws_cmdline_option(argc, argv, "-s")))
		ads = p; //Dirección IP del servicior websockets moemntos: <dirección ip donde corre el servidor momentos websockets

	if ((p = lws_cmdline_option(argc, argv, "-i")))
		iface = p;

    /* Imprime en pantalla como se esta ejecutando el programa */
	lwsl_user("options %d, ads %s\n", options, ads);

    /* Reservamos la memoria para la estructura de información que le pasamos respecto al
       servidor al que nos vamos a conectar - La inicializamos */
	memset(&info, 0, sizeof info); /* otherwise uninitialized garbage */
	info.port = CONTEXT_PORT_NO_LISTEN;
	info.protocols = protocols;
	info.pvo = &pvo;
	if (!lws_cmdline_option(argc, argv, "-n"))
		info.extensions = extensions;
	info.pt_serv_buf_size = 32 * 1024;
	info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT |
		       LWS_SERVER_OPTION_VALIDATE_UTF8;

	if (lws_cmdline_option(argc, argv, "--libuv"))
		info.options |= LWS_SERVER_OPTION_LIBUV;
	else
		signal(SIGINT, sigint_handler);

    /* Se crea el contexto de conexión necesaria para la libreria libwebsockets */
	context = lws_create_context(&info);
	if (!context) {
		lwsl_err("Falló la inicialización del contexto para conectarse al servidor momentos\n");
		return 1;
	}

	while (!lws_service(context, 1000) && !interrupted)
		; //Ciclo infinito esperando notificaciones de disponibilidad de fotos del servidor momentos

    //Limpiamos la memoria una vez que se termino o detuvimos el cliente momentos
	lws_context_destroy(context);

	n = (options & 1) ? interrupted != 2 : interrupted == 3;
	lwsl_user("Completed %d %s\n", interrupted, !n ? "OK" : "failed");

	return n;
}
