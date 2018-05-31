<?hh // strict
namespace Usox\HackTTP;

use Facebook\Experimental\Http\Message\UriInterface;
use HH\Lib\{C, Str};

final class Uri implements UriInterface {

	const HTTP_DEFAULT_HOST = 'localhost';

	private static dict<string, int> $default_ports = dict[
		'http'  => 80,
		'https' => 443,
		'ftp' => 21,
		'gopher' => 70,
		'nntp' => 119,
		'news' => 119,
		'telnet' => 23,
		'tn3270' => 23,
		'imap' => 143,
		'pop' => 110,
		'ldap' => 389,
	];

	private static string $char_unreserved = 'a-zA-Z0-9_\-\.~';

	private static string $char_sub_delimeters = '!\$&\'\(\)\*\+,;=';

	private ?string $scheme;

	private ?string $user_name;

	private ?string $user_password;

	private ?string $host;

	private ?int $port;

	private ?string $path;

	private ?string $query;

	private ?string $fragment;

	public function __construct(?string $uri = null): void {
		if ($uri !== null) {
			$this->loadFromUri($uri);
		}
	}

	public function __toString(): string {
		$uri = '';

		if ($this->scheme !== null) {
			$uri .= $this->scheme.':';
		}

		$authority = $this->getAuthority();

		if ($authority !== '' || $this->scheme === 'file') {
			$uri .= '//' . $authority;
		}

		$uri .= $this->path;

		if ($this->query !== null) {
			$uri .= '?'.$this->query;
		}

		if ($this->fragment !== null) {
			$uri .= '#'.$this->fragment;
		}

		return $uri;
	}

	public function getScheme(): ?string {
		return $this->scheme;
	}

	public function getAuthority(): string {
		$authority = (string) $this->host;

		$user_info = '';
		if ($this->user_name !== null && $this->user_name !== '') {
			$user_info = $this->user_name;

			if ($this->user_password !== null) {
				$user_info .= Str\format(':%s', $this->user_password);
			}

			$authority = Str\format(
				'%s@%s',
				$user_info,
				$authority
			);
		}

		if ($this->port !== null) {
			$authority .= Str\format(':%d', $this->port);
		}

		return $authority;
	}

	public function getUserInfo(): ?shape('user' => string, 'pass' => ?string) {
		if ($this->user_name === '') {
			return null;
		}
		return shape(
			'user' => (string) $this->user_name,
			'pass' => $this->user_password
		);
	}

	public function getHost(): ?string {
		return $this->host;
	}

	public function getPort(): ?int	{
		return $this->port;
	}

	public function getPath(): ?string{
		return $this->path;
	}

	public function getQuery(): ?string {
		return $this->query;
	}

	public function getFragment(): ?string {
		return $this->fragment;
	}

	public function withScheme(?string $scheme): this {
		$scheme = $this->filterScheme($scheme);

		if ($this->scheme === $scheme) {
			return $this;
		}

		$new = clone $this;
		$new->scheme = $scheme;
		$new->removeDefaultPort();
		$new->validateState();

		return $new;
	}

	public function withUserInfo(?string $user, ?string $password = null): this {
		if ($this->user_name === $user && $this->user_password === $password) {
			return $this;
		}

		$new = clone $this;
		$new->user_name = $user;
		$new->user_password = $password;
		$new->validateState();

		return $new;
	}

	public function withHost(?string $host = null): this {
		$host = $this->filterHost($host);

		if ($this->host === $host) {
			return $this;
		}

		$new = clone $this;
		$new->host = $host;
		$new->validateState();

		return $new;
	}

	public function withPort(?int $port = null): this {
		$port = $this->filterPort($port);

		if ($this->port === $port) {
			return $this;
		}

		$new = clone $this;
		$new->port = $port;
		$new->removeDefaultPort();
		$new->validateState();

		return $new;
	}

	public function withPath(?string $path = null): this {
		$path = $this->filterPath($path);

		if ($this->path === $path) {
			return $this;
		}

		$new = clone $this;
		$new->path = $path;
		$new->validateState();

		return $new;
	}

	public function withQuery(?string  $query = null): this {
		$query = $this->filterQueryAndFragment($query);

		if ($this->query === $query) {
			return $this;
		}

		$new = clone $this;
		$new->query = $query;

		return $new;
	}

	public function withFragment(?string $fragment = null): this {
		$fragment = $this->filterQueryAndFragment($fragment);

		if ($this->fragment === $fragment) {
			return $this;
		}

		$new = clone $this;
		$new->fragment = $fragment;

		return $new;
	}

	private function loadFromUri(string $uri): void {
		$parts = \parse_url($uri);
		if ($parts === false) {
			throw new \InvalidArgumentException("Unable to parse URI: $uri");
		}
		if (C\contains_key($parts, 'scheme')) {
			$this->scheme = $this->filterScheme((string) $parts['scheme']);
		}
		if (C\contains_key($parts, 'host')) {
			$this->host = $this->filterHost((string) $parts['host']);
		}
		if (C\contains_key($parts, 'port') && $parts['port'] !== null) {
			$this->port = $this->filterPort((int) $parts['port']);
		}
		if (C\contains_key($parts, 'path')) {
			$this->path = $this->filterPath((string) $parts['path']);
		}
		if (C\contains_key($parts, 'query')) {
			$this->query = $this->filterQueryAndFragment((string) $parts['query']);
		}
		if (C\contains_key($parts, 'fragment')) {
			$this->fragment = $this->filterQueryAndFragment((string) $parts['fragment']);
		}
		if (C\contains_key($parts, 'user')) {
			$this->user_name = (string) $parts['user'];
		}
		if (C\contains_key($parts, 'pass')) {
			$this->user_password = (string) $parts['pass'];
		}

		$this->removeDefaultPort();
	}

	private function filterScheme(?string $scheme): ?string {
		if ($scheme === null) {
			return null;
		}
		return Str\lowercase($scheme);
	}

	private function filterHost(?string $host): ?string {
		if ($host === null) {
			return null;
		}
		return Str\lowercase($host);
	}

	private function filterPort(?int $port): ?int {
		if ($port === null) {
			return null;
		}

		$max_port = 65535;

		if (1 > $port || $max_port < $port) {
			throw new \InvalidArgumentException(
				Str\format('Invalid port: %d. Must be between 1 and %d', $port, $max_port)
			);
		}
		return $port;
	}

	private function removeDefaultPort(): void {
		if (
			$this->port !== null &&
			C\contains_key(static::$default_ports, (string) $this->scheme) && $this->port === static::$default_ports[(string) $this->scheme]
		) {
			$this->port = null;
		}
	}

	private function filterPath(?string $path): ?string {
		if ($path === null) {
			return null;
		}
		return \preg_replace_callback(
			'/(?:[^' . static::$char_unreserved . static::$char_sub_delimeters . '%:@\/]++|%(?![A-Fa-f0-9]{2}))/',
			function ($match) {
				return \rawurlencode($match[0]);
			},
			$path
		);
	}

	private function filterQueryAndFragment(?string $str): ?string {
		if ($str === null) {
			return null;
		}
		return \preg_replace_callback(
			'/(?:[^' . static::$char_unreserved . static::$char_sub_delimeters . '%:@\/\?]++|%(?![A-Fa-f0-9]{2}))/',
			function ($match) {
				return \rawurlencode($match[0]);
			},
			$str
		);
	}

	private function validateState(): void {
		if ($this->host === null && ($this->scheme === 'http' || $this->scheme === 'https')) {
			$this->host = static::HTTP_DEFAULT_HOST;
		}
		$path = (string) $this->path;

		if ($this->getAuthority() === '') {
			if (0 === Str\search($path, '//')) {
				throw new \InvalidArgumentException('The path of a URI without an authority must not start with two slashes "//"');
			}
			if ($this->scheme === null && null !== Str\search(\explode('/', $path, 2)[0], ':')) {
				throw new \InvalidArgumentException('A relative URI must not have a path beginning with a segment containing a colon');
			}
		} elseif ($path !== '' && Str\search($path, '/') !== 0) {
			throw new \InvalidArgumentException('The path of a URI with an authority must start with a slash "/" or be empty');
		}
	}
}
