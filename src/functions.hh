<?hh // strict

namespace Usox\HackTTP;

use namespace Facebook\Experimental\Http\Message;
use namespace HH\Lib\{Experimental\Filesystem, Experimental\IO};

/**
 * We still have to rely on good old php's super globals, so provide a
 * nice convenience method for all the users out there
 */
function createServerRequestFromGlobals(): Message\ServerRequestInterface {
	/* HH_FIXME[2050] */
	$server_params = dict($_SERVER);
	/* HH_FIXME[2050] */
	$post_vars = dict($_POST);
	/* HH_FIXME[2050] */
	$get_vars = dict($_GET);
	/* HH_FIXME[2050] */
	$cookies = dict($_COOKIE);

	return createServerRequest(
		Message\HTTPMethod::assert(
			$server_params['REQUEST_METHOD'] ?? Message\HTTPMethod::GET,
		),
		createUri($server_params['REQUEST_URI'] ?? null),
		$server_params,
		IO\request_input(),
	)
		->withParsedBody($post_vars)
		->withCookieParams($cookies)
		->withQueryParams($get_vars)
		->withUploadedFiles(
			Marshaler\UploadedFileMarshaler::batchMarshalPhpFilesArray(),
		);
}

function createServerRequest(
	Message\HTTPMethod $method,
	Message\UriInterface $uri,
	dict<string, string> $server_params,
	IO\ReadHandle $request_input,
): Message\ServerRequestInterface {
	return new ServerRequest($method, $uri, $server_params, $request_input);
}

function createUri(?string $uri = null): Message\UriInterface {
	return new Uri($uri);
}

function createResponse(
	int $code = 200,
	string $reason = '',
): Message\ResponseInterface {
	$write_handle = Filesystem\open_write_only_non_disposable(
		\sys_get_temp_dir().'/'.\bin2hex(\random_bytes(16)),
		Filesystem\FileWriteMode::MUST_CREATE
	);
	return new Response($write_handle, $code, $reason);
}
