export class GenericResourceFetcher {
  fetchResource(
    url: string,
    callback: (error: Error | null, data: ArrayBuffer | null) => void
  ) {
    fetch(url)
      .then((response) => response.arrayBuffer())
      .then((data) => callback(null, data))
      .catch((error) => callback(error, null));
  }
}
