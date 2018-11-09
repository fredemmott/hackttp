<?hh // strict

namespace Usox\HackTTP;

use namespace Facebook\Experimental\Http\Message;
use type Usox\HackHttpFactory\RequestFactoryInterface;

final class RequestFactory implements RequestFactoryInterface {

  public function createRequest(
    Message\HTTPMethod $method,
    Message\UriInterface $uri,
  ): Message\RequestInterface {
    return new Request($method, $uri, dict[]);
  }
}
