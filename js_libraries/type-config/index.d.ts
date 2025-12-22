export * from './types'
import { CompilerOptions, Config } from './types'

// Copyied from https://github.com/type-challenges/type-challenges/issues/737#issuecomment-3486953045
type UnionToIntersection<U> = (
  U extends unknown ? (arg: U) => unknown : never
) extends (arg: infer I) => void
  ? I
  : never;

type LastInUnion<T> = UnionToIntersection<
  T extends unknown ? () => T : never
> extends () => infer R
  ? R
  : never;

type UnionToTuple<U, T extends unknown[] = []> = [U] extends [T[number]]
  ? T
  : UnionToTuple<U, [...T, LastInUnion<Exclude<U, T[number]>>]>;


export type CompilerOptionsKeys = UnionToTuple<keyof CompilerOptions>
export type ConfigKeys = UnionToTuple<keyof Config>

export const compilerOptionsKeys: CompilerOptionsKeys
export const configKeys: ConfigKeys
