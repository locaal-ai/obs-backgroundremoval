import { getLatestReleaseMetadata } from '../../lib/github-release';

/** @type {import('./$types').PageLoad} */
export async function load() {
	return { metadata: await getLatestReleaseMetadata() };
}
